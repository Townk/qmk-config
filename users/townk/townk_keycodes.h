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
// Can't use the proper safe range number here if I want my custom keys to show
// up on Vial or Keybard.
#    define RANGE_START QK_KB_20
#else
#    define RANGE_START SAFE_RANGE
#endif

#define MKC_BKTAB   S(KC_TAB)      // Back Tab (⇧ ⇥)
#define MKC_APPWIN  C(G(KC_DOWN))  // Show windows from current app (⌃ ⌘ ↓)
#define MKC_ALLWIN  C(G(KC_UP))    // Show all windows from all apps (Mission Control) (⌃ ⌘ ↑)
#define MKC_DKTN    C(G(KC_RGHT))  // Next Desktop (⌃ ⌘ →)
#define MKC_DKTP    C(G(KC_LEFT))  // Previous Desktop (⌃ ⌘ ←)
#define MKC_SHDKT   A(G(KC_DOWN))  // Show Desktop (⌥ ⌘ ↓)
#define MKC_SHNOT   A(G(KC_UP))    // Show Notification Center (⌥ ⌘ ↑)
#define MKC_SPAPP   A(G(KC_LEFT))  // Spotlight Apps (⌥ ⌘ ←)
#define MKC_SPFND   A(G(KC_RIGHT)) // Spotlight Find (⌥ ⌘ →)

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
