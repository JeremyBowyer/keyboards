#include QMK_KEYBOARD_H

#include "rgblight.h"

#define RELAY_PIN 11
#define RELAY_ON 1
#define RELAY_OFF 0

bool last_led_state;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_F13, KC_F14, KC_F15, KC_F16, KC_F19, KC_F17, KC_F18, KC_F22, KC_F20, KC_F21),
};


void keyboard_post_init_user(void) {
debug_enable=true;
    debug_matrix=true;

    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    rgblight_sethsv_noeeprom(200, 255, 80);

    last_led_state = rgblight_get_val() > 0 ? RELAY_ON : RELAY_OFF;
    gpio_set_pin_output(RELAY_PIN);
    gpio_write_pin(RELAY_PIN, last_led_state);
}

void housekeeping_task_user(void) {
    bool leds_on = rgblight_get_val() > 0 ? RELAY_ON : RELAY_OFF;
    if (leds_on != last_led_state) {
        last_led_state = leds_on;
        gpio_write_pin(RELAY_PIN, leds_on);
    }
}

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_90;
}

bool oled_task_user(void) {
    static const char PROGMEM raw_logo[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,128,224,248,200,220,252,246,222,254,254,246,238,254,252,252,248,240,224,128,0,0,0,0,0,0,0,0,0,0,255,127,254,241,103,159,63,63,127,255,255,255,255,127,63,223,15,135,243,251,225,17,252,254,0,0,0,0,
        0,0,0,0,7,15,11,15,16,63,59,32,63,56,7,7,7,48,62,63,48,15,59,62,27,30,15,15,0,0,0,0,192,224,16,16,16,0,128,192,64,64,128,0,128,64,64,0,192,64,64,192,64,64,128,0,128,64,64,128,0,128,64,64,0,1,2,2,2,0,1,3,2,2,1,0,2,3,3,0,3,0,0,3,0,0,3,0,3,2,2,1,0,2,3,3,0,0,0,0,0,0,0,0,0,0,0,0,0,16,16,124,16,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,128,0,128,0,0,128,0,0,128,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,160,160,160,248,252,252,255,28,255,252,252,31,252,252,255,28,255,252,252,248,160,160,160,0,0,0,0,0,0,0,0,0,164,164,164,255,255,255,255,240,239,223,223,0,223,223,239,240,255,255,255,255,164,164,164,0,0,0,0,0,0,0,0,0,0,0,0,3,7,7,63,7,63,7,7,63,7,7,63,7,63,7,7,3,0,0,0,0,0,0,0,0,
        0,0,0,128,192,96,96,96,192,128,0,224,224,192,0,0,0,192,224,224,0,224,224,0,128,192,224,96,0,0,0,0,0,0,0,15,31,48,48,112,223,143,0,63,63,3,15,28,15,3,63,63,0,63,63,7,15,29,56,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };

    oled_write_raw_P(raw_logo, sizeof(raw_logo)); // Show Cosmos + QMK logo
    return false;
}
