#include QMK_KEYBOARD_H

#include "rgblight.h"
#include "vista508.h"

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

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) {
        tap_code(clockwise ? KC_VOLU : KC_VOLD);
    }
    return false;
}

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

#ifdef VIK_VISTA508
    vista508_init();
#endif
}

// QMK calls this regularly. Update the relay only when RGB brightness crosses zero.
// This follows the brightness value, not the separate RGB enabled/disabled flag.
void housekeeping_task_user(void) {
    bool leds_on = rgblight_get_val() > 0 ? RELAY_ON : RELAY_OFF;
    if (leds_on != last_led_state) {
        last_led_state = leds_on;
        gpio_write_pin(RELAY_PIN, leds_on);
    }

#ifdef VIK_VISTA508
    vista508_task();
#endif
}
