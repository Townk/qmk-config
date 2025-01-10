/* Copyright (C) 2024 Thiago Alves (https://github.com/townk)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>

#include QMK_KEYBOARD_H
#include "quantum_keycodes.h"
#include "townk.h"

key_override_t lbracket_as_tilde_override = ko_make_basic(MOD_MASK_GUI, KC_LBRC, KC_TILD);

void keyboard_post_init_user(void) {
    // Customise these values if you need to debug the matrix
    // debug_enable=true;
    // debug_matrix=true;
    // debug_keyboard=true;
    // debug_mouse=true;
    setup_rgb_light_layer();

    // Initial configuration
    global_saved_values.left_scroll       = true;
    global_saved_values.right_scroll      = false;
    global_saved_values.disable_achordion = true;
    global_saved_values.left_dpi_index    = MOUSE_DPI_400;
    global_saved_values.right_dpi_index   = MOUSE_DPI_1200;
    global_saved_values.mh_timer_index    = MOUSE_LAYER_TIMEOUT_NONE;
}

const uint16_t PROGMEM keymaps[DYNAMIC_KEYMAP_LAYER_COUNT][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        /*     Center          North      East      South       West       (XXX)        */
        /*R1*/ CKC_N,          KC_L,      KC_K,     KC_H,       KC_M,      XXXXXXX,
        /*R2*/ CKC_E,          KC_U,      KC_MINS,  KC_COMMA,   KC_J,      XXXXXXX,
        /*R3*/ CKC_I,          KC_Y,      KC_EQL,   KC_DOT,     KC_RPRN,   XXXXXXX,
        /*R4*/ CKC_O,          KC_SCLN,   KC_QUOT,  KC_SLASH,   KC_RBRC,   XXXXXXX,

        /*L1*/ CKC_T,          KC_P,      KC_G,     KC_D,       KC_V,      XXXXXXX,
        /*L2*/ CKC_S,          KC_F,      KC_B,     KC_C,       KC_EXLM,   XXXXXXX,
        /*L3*/ CKC_R,          KC_W,      KC_LPRN,  KC_X,       KC_BSLS,   XXXXXXX,
        /*L4*/ CKC_A,          KC_Q,      KC_LBRC,  KC_Z,       KC_GRV,    XXXXXXX,

        /*     Down            Pad        Up        Nail        Knuckle    Double Down  */
        /*RT*/ KC_ENTER,       CKC_SPC,   KC_ESC,   CKC_BKTAB,  MO(_MBO),  QK_CAPS_WORD_TOGGLE,
        /*LT*/ CKC_SMART_SFT,  CKC_BSPC,  QK_REP,   CKC_TAB,    MO(_MBO),  QK_CAPS_WORD_TOGGLE
        ),

    [_NAV] = LAYOUT(
        /*     Center     North           East          South     West          (XXX)        */
        /*R1*/ KC_LEFT,   C(G(KC_LEFT)),  KC_END,      KC_H,     KC_HOME,       XXXXXXX,
        /*R2*/ KC_DOWN,   C(G(KC_DOWN)),  A(KC_RGHT),  KC_J,     A(KC_LEFT),    XXXXXXX,
        /*R3*/ KC_UP,     C(G(KC_UP)),    C(KC_RGHT),  KC_K,     C(KC_LEFT),    XXXXXXX,
        /*R4*/ KC_RIGHT,  C(G(KC_RGHT)),  KC_PGDN,     KC_L,     KC_PGUP,       XXXXXXX,

        /*L1*/ KC_LSFT,   XXXXXXX,        XXXXXXX,     XXXXXXX,  XXXXXXX,       XXXXXXX,
        /*L2*/ KC_LGUI,   XXXXXXX,        XXXXXXX,     XXXXXXX,  XXXXXXX,       XXXXXXX,
        /*L3*/ KC_LALT,   XXXXXXX,        G(KC_TAB),   XXXXXXX,  G(S(KC_TAB)),  XXXXXXX,
        /*L4*/ KC_LCTL,   XXXXXXX,        G(KC_GRV),   XXXXXXX,  G(KC_TILD),    XXXXXXX,

        /*     Down       Pad             Up           Nail      Knuckle        Double Down  */
        /*RT*/ _______,   _______,        _______,     XXXXXXX,  _______,       XXXXXXX,
        /*LT*/ XXXXXXX,   _______,        _______,     XXXXXXX,  _______,       XXXXXXX
        ),

    [_NUM] = LAYOUT(
        /*     Center    North     East      South     West          (XXX)        */
        /*R1*/ KC_RSFT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,      XXXXXXX,
        /*R2*/ KC_RGUI,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,      XXXXXXX,
        /*R3*/ KC_RALT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,      XXXXXXX,
        /*R4*/ KC_RCTL,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,      XXXXXXX,

        /*L1*/ KC_KP_9,  KC_PPLS,  KC_KP_0,  KC_KP_4,  KC_PDOT,      XXXXXXX,
        /*L2*/ KC_KP_8,  KC_PMNS,  KC_RPRN,  KC_KP_3,  KC_LPRN,      XXXXXXX,
        /*L3*/ KC_KP_7,  KC_PAST,  KC_PEQL,  KC_KP_2,  KC_CIRC,      XXXXXXX,
        /*L4*/ KC_KP_6,  KC_PSLS,  KC_PCMM,  KC_KP_1,  KC_KP_5,      XXXXXXX,

        /*     Down      Pad       Up        Nail      Knuckle       Double Down  */
        /*RT*/ _______,  _______,  _______,  _______,  _______,      XXXXXXX,
        /*LT*/ KC_PENT,  _______,  _______,  _______,  _______,      XXXXXXX
        ),

    [_SYM] = LAYOUT(
        /*     Center    North      East      South     West      (XXX)        */
        /*R1*/ KC_LCBR,  KC_LT,     KC_CIRC,  KC_LBRC,  KC_DLR,   XXXXXXX,
        /*R2*/ KC_COLN,  KC_EQUAL,  KC_PIPE,  KC_ASTR,  KC_AMPR,  XXXXXXX,
        /*R3*/ KC_RCBR,  KC_GT,     KC_PERC,  KC_RBRC,  KC_EXLM,  XXXXXXX,
        /*R4*/ KC_AT,    KC_UNDS,   KC_BSLS,  KC_HASH,  KC_SLSH,  XXXXXXX,

        /*L1*/ KC_LSFT,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*L2*/ KC_LGUI,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*L3*/ KC_LALT,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*L4*/ KC_LCTL,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,

        /*     Down      Pad        Up        Nail      Knuckle   Double Down  */
        /*RT*/ _______,  _______,   _______,  XXXXXXX,  _______,  XXXXXXX,
        /*LT*/ XXXXXXX,  _______,   _______,  XXXXXXX,  _______,  XXXXXXX
        ),

    [_FUN] = LAYOUT(
        /*     Center    North     East      South     West      (XXX)        */
        /*R1*/ KC_RSFT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R2*/ KC_RGUI,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R3*/ KC_RALT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R4*/ KC_RCTL,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,

        /*L1*/ KC_F11,   KC_F20,   KC_F12,   KC_F16,   KC_F10,   XXXXXXX,
        /*L2*/ KC_F8,    KC_F19,   KC_F9,    KC_F15,   KC_F7,    XXXXXXX,
        /*L3*/ KC_F5,    KC_F18,   KC_F6,    KC_F14,   KC_F4,    XXXXXXX,
        /*L4*/ KC_F2,    KC_F17,   KC_F3,    KC_F13,   KC_F1,    XXXXXXX,

        /*     Down      Pad       Up        Nail      Knuckle   Double Down  */
        /*RT*/ _______,  _______,  _______,  XXXXXXX,  _______,  XXXXXXX,
        /*LT*/ XXXXXXX,  _______,  _______,  XXXXXXX,  _______,  XXXXXXX
        ),

    [_SYS] = LAYOUT(
        /*     Center                   North                 East      South              West      (XXX)        */
        /*R1*/ _______,                 _______,              _______,  _______,           _______,  XXXXXXX,
        /*R2*/ _______,                 RGB_VAI,              _______,  RGB_VAD,           _______,  XXXXXXX,
        /*R3*/ _______,                 _______,              _______,  _______,           _______,  XXXXXXX,
        /*R4*/ _______,                 _______,              _______,  _______,           _______,  XXXXXXX,

        /*L1*/ SV_OUTPUT_STATUS,        _______,              _______,  _______,           _______,  XXXXXXX,
        /*L2*/ SV_RIGHT_SCROLL_TOGGLE,  SV_RIGHT_DPI_INC,     _______,  SV_RIGHT_DPI_DEC,  _______,  XXXXXXX,
        /*L3*/ SV_LEFT_SCROLL_TOGGLE,   SV_LEFT_DPI_INC,      _______,  SV_LEFT_DPI_DEC,   _______,  XXXXXXX,
        /*L4*/ SV_MH_CHANGE_TIMEOUTS,   SV_TOGGLE_ACHORDION,  _______,  _______,           _______,  XXXXXXX,

        /*     Down                     Pad                   Up        Nail               Knuckle   Double Down  */
        /*RT*/ _______,                 _______,              _______,  _______,           _______,  _______,
        /*LT*/ _______,                 _______,              _______,  _______,           _______,  _______
      ),

    [_MBO] = LAYOUT(
        /*     Center     North        East      South     West      (XXX)        */
        /*R1*/ _______,   _______,     _______,  _______,  _______,  XXXXXXX,
        /*R2*/ _______,   _______,     _______,  _______,  _______,  XXXXXXX,
        /*R3*/ _______,   _______,     _______,  _______,  _______,  XXXXXXX,
        /*R4*/ _______,   _______,     _______,  _______,  _______,  XXXXXXX,

        /*L1*/ _______,  _______,      _______,  KC_BTN1,  _______,  XXXXXXX,
        /*L2*/ _______,  SV_SNIPER_2,  _______,  KC_BTN3,  _______,  XXXXXXX,
        /*L3*/ _______,  SV_SNIPER_3,  _______,  KC_BTN2,  _______,  XXXXXXX,
        /*L4*/ _______,  SV_SNIPER_5,  _______,  KC_BTN4,  _______,  XXXXXXX,

        /*     Down       Pad          Up        Nail      Knuckle   Double Down  */
        /*RT*/ _______,   KC_BTN1,     TO(0),  KC_BTN2,  KC_BTN3,  _______,
        /*LT*/ _______,   _______,     TO(0),  _______,  _______,  _______
        )
};
