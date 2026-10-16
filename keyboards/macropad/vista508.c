#include "quantum.h"
#include "spi_master.h"
#include "vista508.h"

#include <string.h>

// Vista508: Sharp LS0xx memory LCD, 144x168, same bus protocol as the Rolio
// ZMK shield (sharp,ls0xx-vcom). Chip select is active high. The RP2040 SPI
// peripheral can only shift MSB first, so each byte is reversed before it is
// sent. Pixel bit 0 in the framebuffer is the leftmost pixel of that group.

#define VISTA508_WIDTH 144
#define VISTA508_HEIGHT 168
#define VISTA508_STRIDE (VISTA508_WIDTH / 8)
#define VISTA508_CS_PIN VIK_SPI_CS

#define VISTA_CMD_WRITE 0x01
#define VISTA_CMD_VCOM 0x02
#define VISTA_CMD_CLEAR 0x04

#define VISTA_VCOM_MS 17
#define VISTA_FRAME_MS 180

static uint8_t framebuffer[VISTA508_STRIDE * VISTA508_HEIGHT];
static bool vcom_level;
static uint16_t vcom_timer;
static uint16_t frame_timer;
static uint8_t bar_x;

static uint8_t reverse_bits(uint8_t value) {
    value = (uint8_t)((value & 0xF0) >> 4 | (value & 0x0F) << 4);
    value = (uint8_t)((value & 0xCC) >> 2 | (value & 0x33) << 2);
    value = (uint8_t)((value & 0xAA) >> 1 | (value & 0x55) << 1);
    return value;
}

static void set_pixel(int x, int y, bool on) {
    if ((unsigned)x >= VISTA508_WIDTH || (unsigned)y >= VISTA508_HEIGHT) {
        return;
    }
    uint8_t *cell = &framebuffer[y * VISTA508_STRIDE + (x >> 3)];
    uint8_t mask = (uint8_t)(1u << (x & 7));
    if (on) {
        *cell |= mask;
    } else {
        *cell &= (uint8_t)~mask;
    }
}

static void fill_rect(int x, int y, int width, int height, bool on) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            set_pixel(x + col, y + row, on);
        }
    }
}

// 5x7 glyphs. Bit 0 of each row is the leftmost pixel.
static const uint8_t glyph_v[7] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
static const uint8_t glyph_i[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F};
static const uint8_t glyph_s[7] = {0x1E, 0x01, 0x01, 0x0E, 0x10, 0x10, 0x0F};
static const uint8_t glyph_t[7] = {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
static const uint8_t glyph_a[7] = {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
static const uint8_t glyph_5[7] = {0x1F, 0x01, 0x01, 0x0F, 0x10, 0x10, 0x0F};
static const uint8_t glyph_0[7] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
static const uint8_t glyph_8[7] = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};

static void draw_glyph(int x, int y, const uint8_t *rows, int scale) {
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (rows[row] & (1u << col)) {
                fill_rect(x + col * scale, y + row * scale, scale, scale, false);
            }
        }
    }
}

static void draw_test_pattern(void) {
    memset(framebuffer, 0xFF, sizeof(framebuffer));
    fill_rect(0, 0, VISTA508_WIDTH, 3, false);
    fill_rect(0, VISTA508_HEIGHT - 3, VISTA508_WIDTH, 3, false);
    fill_rect(0, 0, 3, VISTA508_HEIGHT, false);
    fill_rect(VISTA508_WIDTH - 3, 0, 3, VISTA508_HEIGHT, false);

    const uint8_t *glyphs[] = {glyph_v, glyph_i, glyph_s, glyph_t, glyph_a, glyph_5, glyph_0, glyph_8};
    const int scale = 3;
    const int step = 6 * scale;
    int x = (VISTA508_WIDTH - (int)(sizeof(glyphs) / sizeof(glyphs[0])) * step) / 2;
    for (size_t index = 0; index < sizeof(glyphs) / sizeof(glyphs[0]); index++) {
        draw_glyph(x, 48, glyphs[index], scale);
        x += step;
    }

    fill_rect(4 + bar_x, 130, 28, 18, false);
}

static bool vista_begin(void) {
    spi_start_config_t config = {
        .slave_pin = VISTA508_CS_PIN,
        .lsb_first = false,
        .mode = 0,
        .divisor = 128,
        .cs_active_low = false,
    };
    if (!spi_start_extended(&config)) {
        return false;
    }
    wait_us(6);
    return true;
}

static void vista_end(void) {
    spi_stop();
    wait_us(4);
}

static uint8_t next_command(uint8_t command) {
    if (vcom_level) {
        command |= VISTA_CMD_VCOM;
    }
    vcom_level = !vcom_level;
    return reverse_bits(command);
}

static void vista_command(uint8_t command) {
    uint8_t buffer[2] = {next_command(command), 0x00};
    if (!vista_begin()) {
        return;
    }
    spi_transmit(buffer, sizeof(buffer));
    vista_end();
}

static void vista_flush(void) {
    uint8_t command = next_command(VISTA_CMD_WRITE);
    if (!vista_begin()) {
        return;
    }
    spi_transmit(&command, 1);

    for (uint8_t line = 1; line <= VISTA508_HEIGHT; line++) {
        uint8_t packet[1 + VISTA508_STRIDE + 1];
        packet[0] = reverse_bits(line);
        const uint8_t *source = &framebuffer[(line - 1) * VISTA508_STRIDE];
        for (uint8_t column = 0; column < VISTA508_STRIDE; column++) {
            packet[1 + column] = reverse_bits(source[column]);
        }
        packet[1 + VISTA508_STRIDE] = 0x00;
        spi_transmit(packet, sizeof(packet));
    }

    uint8_t trailer = 0x00;
    spi_transmit(&trailer, 1);
    vista_end();
}

void vista508_init(void) {
    gpio_set_pin_output(VISTA508_CS_PIN);
    gpio_write_pin(VISTA508_CS_PIN, 0);
    spi_init();

    vcom_level = false;
    vista_command(VISTA_CMD_CLEAR);
    bar_x = 0;
    draw_test_pattern();
    vista_flush();
    vcom_timer = timer_read();
    frame_timer = timer_read();
}

void vista508_task(void) {
    if (timer_elapsed(frame_timer) >= VISTA_FRAME_MS) {
        frame_timer = timer_read();
        bar_x += 6;
        if (bar_x > 108) {
            bar_x = 0;
        }
        draw_test_pattern();
        vista_flush();
        vcom_timer = timer_read();
        return;
    }

    if (timer_elapsed(vcom_timer) >= VISTA_VCOM_MS) {
        vcom_timer = timer_read();
        vista_command(0x00);
    }
}
