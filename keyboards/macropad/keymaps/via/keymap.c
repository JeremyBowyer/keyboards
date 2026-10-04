#include QMK_KEYBOARD_H

#include "rgblight.h"

#define RELAY_PIN 11
#define RELAY_ON 1
#define RELAY_OFF 0
#define F20_HOLD_TERM 190

bool last_led_state;
static bool f20_pressed;
static bool f20_holding;
static uint16_t f20_timer;

// Layer 0. LAYOUT maps arguments to matrix positions in keyboard.json order.
// Physical arrangement (not argument order):
//   F13  F14
//   F15  F16
//   F17  F18  Backspace
//   F20  F21  Enter
// Backspace and Enter must come before the other keys in their respective rows.
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_F13, KC_F14,
        KC_F15, KC_F16,
                        KC_BSPC,
        KC_F17, KC_F18,
                        KC_ENT,
        KC_F20, KC_F21
    ),
};

// QMK calls this after matrix scans. Hold Shift+F20 after 190 ms, once per press.
void matrix_scan_user(void) {
    if (f20_pressed && !f20_holding && timer_elapsed(f20_timer) >= F20_HOLD_TERM) {
        f20_holding = true;
        register_code16(S(KC_F20));
    }
}

// QMK calls this for key presses/releases. Pass other keys through unchanged;
// consume F20 events to send Ctrl+F20 on a short tap or Shift+F20 on a hold.
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode != KC_F20) {
        return true;
    }

    if (record->event.pressed) {
        f20_pressed = true;
        f20_holding = false;
        f20_timer = timer_read();
    } else {
        // Resolve the timeout even if no scan occurred between expiry and release.
        matrix_scan_user();
        f20_pressed = false;
        if (f20_holding) {
            unregister_code16(S(KC_F20));
            f20_holding = false;
        } else {
            tap_code16(C(KC_F20));
        }
    }
    return false;
}


// Run once after initialization: enable debugging and initialize the relay output
// on pin 11. Unlike the default keymap, this leaves the RGB settings unchanged.
void keyboard_post_init_user(void) {
    debug_enable=true;
    debug_matrix=true;

    last_led_state = rgblight_get_val() > 0 ? RELAY_ON : RELAY_OFF;
    gpio_set_pin_output(RELAY_PIN);
    gpio_write_pin(RELAY_PIN, last_led_state);
}

// QMK calls this regularly. Update the relay only when RGB brightness crosses zero.
// This follows the brightness value, not the separate RGB enabled/disabled flag.
void housekeeping_task_user(void) {
    bool leds_on = rgblight_get_val() > 0 ? RELAY_ON : RELAY_OFF;
    if (leds_on != last_led_state) {
        last_led_state = leds_on;
        gpio_write_pin(RELAY_PIN, leds_on);
    }
}

// Override the OLED's default orientation with a 90-degree rotation.
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_90;
}

// Draw the stored Cosmos + QMK bitmap whenever QMK runs the OLED task.
// Returning false skips QMK's keyboard-level OLED task for this update.
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
