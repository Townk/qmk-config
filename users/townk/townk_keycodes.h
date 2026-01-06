/* Copyright (C) 2025 Thiago Alves (https://github.com/townk)
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

// Can't use the proper safe range number here (which should be
// `SV_SAFE_RANGE`) if I want my custom keys to show up on Vial or Keybard.
// This overrides two custom key codes defined by Svalboard (`KC_NORMAL_HOLD`
// and `KC_FUNC_HOLD`), but since I'm not using them I believe it's safe to
// do so.
#    define RANGE_START QK_KB_20

#else
#    define RANGE_START SAFE_RANGE
#endif

#define MKC_BKTAB   S(KC_TAB)        ///< Back Tab (⇧ ⇥)
#define MKC_APPWIN  C(G(KC_DOWN))    ///< Show windows from current app (⌃ ⌘ ↓)
#define MKC_ALLWIN  C(G(KC_UP))      ///< Show all windows from all apps (Mission Control) (⌃ ⌘ ↑)
#define MKC_DKTN    C(G(KC_RGHT))    ///< Next Desktop (⌃ ⌘ →)
#define MKC_DKTP    C(G(KC_LEFT))    ///< Previous Desktop (⌃ ⌘ ←)
#define MKC_SHDKT   A(G(KC_DOWN))    ///< Show Desktop (⌥ ⌘ ↓)
#define MKC_SHNOT   A(G(KC_UP))      ///< Show Notification Center (⌥ ⌘ ↑)
#define MKC_SPAPP   A(G(KC_LEFT))    ///< Spotlight Apps (⌥ ⌘ ←)
#define MKC_SPFND   A(G(KC_RIGHT))   ///< Spotlight Find (⌥ ⌘ →)

#define SV_LDPU    SV_LEFT_DPI_INC   ///< Short version of left side mouse DPI increase
#define SV_LDPD    SV_LEFT_DPI_DEC   ///< Short version of left side mouse DPI decrease
#define SV_RDPU    SV_RIGHT_DPI_INC  ///< Short version of right side mouse DPI increase
#define SV_RDPD    SV_RIGHT_DPI_DEC  ///< Short version of right side mouse DPI decrease
#define SV_SOUT    SV_OUTPUT_STATUS  ///< Short version of Svalboard Status Output

enum custom_keycodes {
    /** Backspace on tap, delete on Shift+Tap, navigation layer on hold. */
    CKC_BSPC = RANGE_START,
    CKC_SPC,   ///< Space on tap, numbers layer on hold.
    CKC_TAB,   ///< Tab on tap, symbols layer on hold.
    CKC_BKTAB, ///< Shift+Tab (back-tab) on tap, function keys on hold.
    CKC_SMSFT, ///< One-shot Shift on tap, Shift on hold, Caps-WORD on double-tap.
    MB_SFT,    ///< Mouse button 1 (right-click) on tap, Shift on hold.
    MB_ALT,    ///< Mouse button 2 (lift-click) on tap, Alt on hold.
    MB_GUI,    ///< Mouse button 3 (middle-click) on tap, Gui on hold.
    MB_CTL,    ///< Mouse button 4 on tap, Ctrl on hold.
};

#endif // QMK_USERSPACE_TOWNK_KEYMAPS_H
