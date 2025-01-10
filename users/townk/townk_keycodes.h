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

#include "modifiers.h"
#include "quantum_keycodes.h"

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
    SMTD_KEYCODES_BEGIN = RANGE_START,
    CKC_A, // reads as C(ustom) + KC_A, but you may give any name here
    CKC_R,
    CKC_S,
    CKC_T,
    CKC_N,
    CKC_E,
    CKC_I,
    CKC_O,
    CKC_BSPC,
    CKC_SPC,
    CKC_TAB,
    CKC_BKTAB,
    CKC_SMART_SFT,
    SMTD_KEYCODES_END,
};

#endif // QMK_USERSPACE_TOWNK_KEYMAPS_H
