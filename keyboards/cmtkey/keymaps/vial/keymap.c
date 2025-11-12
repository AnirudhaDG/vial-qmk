// Copyright 2020 QMK / James Young (@noroadsleft)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "tapdance.c"
#include "ws2812.h"
#include "print.h"
#include <stdio.h>


// #define TRACKPAD_RXS 9   // Number of RX electrodes (columns)
// #define TRACKPAD_TXS 4   // Number of TX electrodes (rows)
// #define TRACKPAD_X_RES 1000  // X resolution
// #define TRACKPAD_Y_RES 1000  // Y resolution

void keyboard_post_init_user(void) {
  install_tap_dance_entries();
  ws2812_init();
    debug_enable=true;
  debug_matrix=true;
  debug_keyboard=true;
  debug_mouse=true;

  print("string\n");
}





const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, RGB_VAI,
        KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_BSPC, RGB_HUI,
        KC_CAPS, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_UP, KC_ENT, RGB_TOG ,
        TD(0), KC_LCTL, KC_LALT, KC_SPC, KC_NO, KC_NO, KC_DOT, KC_LEFT, KC_DOWN, KC_RGHT, RGB_TOG
    ),
    [1] = LAYOUT(
        KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_BSPC,
        KC_AT, KC_HASH, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_BSPC, KC_ENT,
        KC_CAPS, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_UP, KC_ENT, RGB_MOD,
        TD(0), KC_LCTL, KC_LALT, KC_SPC , KC_DOT, KC_LEFT, KC_DOWN, KC_RGHT, RGB_TOG, KC_NO, KC_NO
    ),

};

