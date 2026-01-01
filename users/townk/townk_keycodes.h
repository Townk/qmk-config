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

#ifndef QMK_USERSPACE_TOWNK_KEYMAPS_H
#define QMK_USERSPACE_TOWNK_KEYMAPS_H

#ifdef SVALBOARD
#    include "keymap_support.h"
#    define RANGE_START SV_SAFE_RANGE
#else
#    define RANGE_START SAFE_RANGE
#endif

// Left side
#define HR_MOD_LEFT_1 KC_LEFT_SHIFT
#define HR_MOD_LEFT_2 KC_LEFT_GUI
#define HR_MOD_LEFT_3 KC_LEFT_ALT
#define HR_MOD_LEFT_4 KC_LEFT_CTRL
#define HR_MOD_LEFT_MODIFIER_1 MOD_LSFT
#define HR_MOD_LEFT_MODIFIER_2 MOD_LGUI
#define HR_MOD_LEFT_MODIFIER_3 MOD_LALT
#define HR_MOD_LEFT_MODIFIER_4 MOD_LCTL

// Right side
#define HR_MOD_RIGHT_1 KC_RIGHT_SHIFT
#define HR_MOD_RIGHT_2 KC_RIGHT_GUI
#define HR_MOD_RIGHT_3 KC_RIGHT_ALT
#define HR_MOD_RIGHT_4 KC_RIGHT_CTRL
#define HR_MOD_RIGHT_MODIFIER_1 MOD_RSFT
#define HR_MOD_RIGHT_MODIFIER_2 MOD_RGUI
#define HR_MOD_RIGHT_MODIFIER_3 MOD_RALT
#define HR_MOD_RIGHT_MODIFIER_4 MOD_RCTL

enum custom_keycodes {
    CKC_BSPC = RANGE_START,
    CKC_SPC,
    CKC_TAB,
    CKC_BKTAB,
    CKC_SMART_SFT,
    MB_SFT, // BTN1 or Shift
    MB_ALT, // BTN2 or Alt
    MB_GUI, // BTN3 or Gui
    MB_CTL, // BTN4 or Ctrl
};

#endif // QMK_USERSPACE_TOWNK_KEYMAPS_H
