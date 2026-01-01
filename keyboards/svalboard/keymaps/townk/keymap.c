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
#include "townk_layers.h"
#include "townk_keycodes.h"
#include "townk_mouse.h"
#include "townk_overrides.h"

void keyboard_post_init_user(void) {
    // Customise these values if you need to debug the matrix
    // debug_enable=true;
    // debug_matrix=true;
    // debug_keyboard=true;
    // debug_mouse=true;
    setup_rgb_light_layer();
    setup_dynamic_keymap();

    // Initial configuration
    global_saved_values.left_scroll       = true;
    global_saved_values.right_scroll      = false;
    global_saved_values.left_dpi_index    = MOUSE_DPI_400;
    global_saved_values.right_dpi_index   = MOUSE_DPI_1200;
    global_saved_values.mh_timer_index    = MOUSE_LAYER_TIMEOUT_NONE;
}

const uint16_t PROGMEM keymaps[DYNAMIC_KEYMAP_LAYER_COUNT][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        /*     Center              North          East          South           West           Double-South */
        /*R1*/ KC_N,           KC_L,      KC_MINS,  KC_H,       KC_M,      HR_MOD_RIGHT_1,
        /*R2*/ KC_E,           KC_U,      KC_EQL,   KC_COMMA,   KC_J,      HR_MOD_RIGHT_2,
        /*R3*/ KC_I,           KC_Y,      KC_EXLM,  KC_DOT,     KC_K,      HR_MOD_RIGHT_3,
        /*R4*/ KC_O,           KC_SCLN,   KC_RBRC,  KC_SLASH,   KC_RPRN,   HR_MOD_RIGHT_4,

        /*L1*/ KC_T,           KC_P,      KC_G,     KC_D,       KC_QUOT,   HR_MOD_LEFT_1,
        /*L2*/ KC_S,           KC_F,      KC_B,     KC_C,       KC_GRV,    HR_MOD_LEFT_2,
        /*L3*/ KC_R,           KC_W,      KC_V,     KC_X,       KC_BSLS,   HR_MOD_LEFT_3,
        /*L4*/ KC_A,           KC_Q,      KC_LPRN,  KC_Z,       KC_LBRC,   HR_MOD_LEFT_4,

        /*     Down                Pad            Up            Nail            Knuckle        Double Down  */
        /*RT*/ KC_ENTER,       CKC_SPC,   KC_ESC,   CKC_BKTAB,  MO(_FUN),  QK_CAPS_WORD_TOGGLE,
        /*LT*/ CKC_SMART_SFT,  CKC_BSPC,  QK_REP,   CKC_TAB,    MO(_SYM),  QK_CAPS_WORD_TOGGLE
        ),

    [_NAV] = LAYOUT(
        /*     Center         North               East             South         West               Double-South */
        /*R1*/ KC_LEFT,   C(G(KC_LEFT)),  KC_END,      KC_H,     KC_HOME,       _______,
        /*R2*/ KC_DOWN,   C(G(KC_DOWN)),  A(KC_RGHT),  KC_J,     A(KC_LEFT),    _______,
        /*R3*/ KC_UP,     C(G(KC_UP)),    C(KC_RGHT),  KC_K,     C(KC_LEFT),    _______,
        /*R4*/ KC_RIGHT,  C(G(KC_RGHT)),  KC_PGDN,     KC_L,     KC_PGUP,       _______,

        /*L1*/ KC_LSFT,   XXXXXXX,        XXXXXXX,     XXXXXXX,  XXXXXXX,       _______,
        /*L2*/ KC_LGUI,   XXXXXXX,        XXXXXXX,     XXXXXXX,  XXXXXXX,       _______,
        /*L3*/ KC_LALT,   XXXXXXX,        G(KC_TAB),   XXXXXXX,  G(S(KC_TAB)),  _______,
        /*L4*/ KC_LCTL,   XXXXXXX,        G(KC_GRV),   XXXXXXX,  G(KC_TILD),    _______,

        /*     Down           Pad                 Up               Nail          Knuckle            Double Down  */
        /*RT*/ _______,   _______,        _______,     XXXXXXX,  _______,       XXXXXXX,
        /*LT*/ XXXXXXX,   _______,        _______,     XXXXXXX,  _______,       XXXXXXX
        ),

    [_NUM] = LAYOUT(
        /*     Center        North         East          South         West          Double-South */
        /*R1*/ KC_RSFT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*R2*/ KC_RGUI,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*R3*/ KC_RALT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*R4*/ KC_RCTL,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,

        /*L1*/ KC_KP_8,  KC_PPLS,  KC_KP_9,  KC_KP_4,  KC_PDOT,  _______,
        /*L2*/ KC_KP_7,  KC_PMNS,  KC_RPRN,  KC_KP_3,  KC_LPRN,  _______,
        /*L3*/ KC_KP_6,  KC_PAST,  KC_PEQL,  KC_KP_2,  KC_CIRC,  _______,
        /*L4*/ KC_KP_5,  KC_PSLS,  KC_COMM,  KC_KP_1,  KC_KP_0,  _______,

        /*     Down          Pad           Up            Nail          Knuckle       Double Down  */
        /*RT*/ _______,  _______,  _______,  _______,  _______,  XXXXXXX,
        /*LT*/ KC_PENT,  _______,  _______,  _______,  _______,  XXXXXXX
        ),

    [_SYM] = LAYOUT(
        /*     Center        North          East          South         West          Double-South */
        /*R1*/ KC_LCBR,  KC_LT,     KC_DLR,   KC_LBRC,  KC_CIRC,   _______,
        /*R2*/ KC_COLN,  KC_EQUAL,  KC_PIPE,  KC_ASTR,  KC_AMPR,  _______,
        /*R3*/ KC_RCBR,  KC_GT,     KC_PERC,  KC_RBRC,  KC_EXLM,  _______,
        /*R4*/ KC_AT,    KC_UNDS,   KC_BSLS,  KC_HASH,  KC_SLSH,  _______,

        /*L1*/ KC_LSFT,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*L2*/ KC_LGUI,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*L3*/ KC_LALT,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*L4*/ KC_LCTL,  XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,

        /*     Down          Pad            Up            Nail          Knuckle       Double Down  */
        /*RT*/ _______,  _______,   _______,  XXXXXXX,  _______,  XXXXXXX,
        /*LT*/ XXXXXXX,  _______,   _______,  XXXXXXX,  _______,  XXXXXXX
        ),

    [_FUN] = LAYOUT(
        /*     Center        North         East          South         West          Double-South */
        /*R1*/ KC_RSFT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*R2*/ KC_RGUI,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*R3*/ KC_RALT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,
        /*R4*/ KC_RCTL,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  _______,

        /*L1*/ KC_F8,    KC_F14,   KC_F9,    KC_F1,   KC_F20,    _______,
        /*L2*/ KC_F7,    KC_F13,   KC_F19,   KC_F2,   KC_F18,    _______,
        /*L3*/ KC_F6,    KC_F12,   KC_F17,   KC_F3,   KC_F16,    _______,
        /*L4*/ KC_F5,    KC_F11,   KC_F15,   KC_F4,   KC_F10,    _______,

        /*     Down          Pad           Up            Nail          Knuckle       Double Down  */
        /*RT*/ _______,  _______,  _______,  XXXXXXX,  _______,  XXXXXXX,
        /*LT*/ XXXXXXX,  _______,  _______,  XXXXXXX,  _______,  XXXXXXX
        ),

    [_SYS] = LAYOUT(
        /*     Center                       North                     East          South                 West           Double-South  */
        /*R1*/ _______,                 _______,              _______,  _______,           _______,  _______,
        /*R2*/ _______,                 RGB_VAI,              _______,  RGB_VAD,           _______,  _______,
        /*R3*/ _______,                 _______,              _______,  _______,           _______,  _______,
        /*R4*/ _______,                 _______,              _______,  _______,           _______,  _______,

        /*L1*/ SV_OUTPUT_STATUS,        _______,              _______,  _______,           _______,  _______,
        /*L2*/ SV_RIGHT_SCROLL_TOGGLE,  SV_RIGHT_DPI_INC,     _______,  SV_RIGHT_DPI_DEC,  _______,  _______,
        /*L3*/ SV_LEFT_SCROLL_TOGGLE,   SV_LEFT_DPI_INC,      _______,  SV_LEFT_DPI_DEC,   _______,  _______,
        /*L4*/ SV_MH_CHANGE_TIMEOUTS,   _______,              _______,  _______,           _______,  _______,

        /*     Down                         Pad                       Up            Nail                   Knuckle       Double Down  */
        /*RT*/ _______,                 _______,              _______,  _______,           _______,  _______,
        /*LT*/ _______,                 _______,              _______,  _______,           _______,  _______
      ),

    [_MBO] = LAYOUT(
        /*     Center         North        East              South             West              Double-South  */
        /*R1*/ _______,   _______, _______,      _______,      _______,      HR_MOD_RIGHT_1,
        /*R2*/ _______,   _______, _______,      _______,      _______,      HR_MOD_RIGHT_2,
        /*R3*/ _______,   _______, _______,      _______,      _______,      HR_MOD_RIGHT_3,
        /*R4*/ _______,   _______, _______,      _______,      _______,      HR_MOD_RIGHT_4,

        /*L1*/ _______,  _______,  _______,      _______,      _______,      MB_SFT,
        /*L2*/ _______,  _______,  _______,      _______,      _______,      MB_GUI,
        /*L3*/ _______,  _______,  _______,      _______,      _______,      MB_ALT,
        /*L4*/ _______,  _______,  _______,      SV_SNIPER_3,  _______,      MB_CTL,

        /*     Down          Pad           Up                Nail              Knuckle           Double Down 1 */
        /*RT*/ _______,  CKC_SPC,  TO(0),        CKC_BKTAB,    SV_SNIPER_3,  _______,
        /*LT*/ KC_LSFT,  CKC_BSPC, SV_SNIPER_5,  CKC_TAB,      KC_LALT,      _______
        )
};
