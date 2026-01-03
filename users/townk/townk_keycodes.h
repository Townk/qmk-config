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

#define MKC_BKTAB   S(KC_TAB)      // Back Tab
#define MKC_APPWIN  C(G(KC_DOWN))  // Show windows from current app
#define MKC_ALLWIN  C(G(KC_UP))    // Show all windows from all apps
#define MKC_DKTN    C(G(KC_RGHT))  // Next Desktop
#define MKC_DKTP    C(G(KC_LEFT))  // Previous Desktop

enum custom_keycodes {
    CKC_BSPC = RANGE_START,
    CKC_SPC,
    CKC_TAB,
    CKC_BKTAB, // Back Tab
    CKC_SMSFT, // Smart Shift
    MB_SFT, // BTN1 or Shift
    MB_ALT, // BTN2 or Alt
    MB_GUI, // BTN3 or Gui
    MB_CTL, // BTN4 or Ctrl
};

#endif // QMK_USERSPACE_TOWNK_KEYMAPS_H
