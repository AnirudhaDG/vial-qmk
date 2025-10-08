#include QMK_KEYBOARD_H

#define TAP_TAPPING_TERM 260
void install_tap_dance_entries(void) {
    vial_tap_dance_entry_t td0 = { KC_NO, // Change layers
                                   MO(1),
                                   TG(1),
                                   KC_NO,
                                   TAP_TAPPING_TERM };
    dynamic_keymap_set_tap_dance(0, &td0); // the first value corresponds to the TD(i) slot
}
