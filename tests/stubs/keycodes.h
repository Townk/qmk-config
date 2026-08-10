/* Host-test stand-in for QMK's quantum/keycodes.h.
 *
 * Real QMK values, because townk_smtd.c switches on keycode RANGES
 * (KC_A ... KC_Z, KC_1 ... KC_0) that are only well-formed if the ordering
 * matches. Never compiled into firmware.
 */
#pragma once

/* LSFT() is already provided by sm_td's harness shim, which the fixture
 * includes first. S() is QMK's short alias for it. */
#ifndef S
#    define S(kc) LSFT(kc)
#endif

enum qmk_keycodes {
    KC_NO   = 0x0000,

    KC_A = 0x0004, KC_B, KC_C, KC_D, KC_E, KC_F, KC_G, KC_H, KC_I,
    KC_J, KC_K, KC_L, KC_M, KC_N, KC_O, KC_P, KC_Q, KC_R,
    KC_S, KC_T, KC_U, KC_V, KC_W, KC_X, KC_Y, KC_Z,

    KC_1 = 0x001E, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0,

    KC_ENTER = 0x0028,
    KC_ESC   = 0x0029,
    KC_BSPC  = 0x002A,
    KC_TAB   = 0x002B,
    KC_SPC   = 0x002C,
    KC_MINS  = 0x002D,

    KC_DEL = 0x004C,

    /* Mouse buttons. macOS sees KC_BTN3 as the middle button. */
    KC_BTN1 = 0x00D1, KC_BTN2, KC_BTN3, KC_BTN4,
    KC_BTN5, KC_BTN6, KC_BTN7, KC_BTN8,

    /* MOD_BIT() masks these with 0x07, yielding 1/2/4/8 as on-device. */
    KC_LCTL = 0x00E0, KC_LSFT, KC_LALT, KC_LGUI,
    KC_RCTL = 0x00E4, KC_RSFT, KC_RALT, KC_RGUI,
};

#define KC_UNDS S(KC_MINS)
