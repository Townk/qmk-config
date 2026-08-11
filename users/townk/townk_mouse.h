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

#ifndef QMK_USERSPACE_TOWNK_MOUSE_H
#define QMK_USERSPACE_TOWNK_MOUSE_H

#include "action.h"

#define MOUSE_DPI_200 0
#define MOUSE_DPI_400 1
#define MOUSE_DPI_800 2
#define MOUSE_DPI_1200 3
#define MOUSE_DPI_1600 4
#define MOUSE_DPI_2400 5

#define MOUSE_LAYER_TIMEOUT_200_MS 0
#define MOUSE_LAYER_TIMEOUT_300_MS 1
#define MOUSE_LAYER_TIMEOUT_400_MS 2
#define MOUSE_LAYER_TIMEOUT_500_MS 3
#define MOUSE_LAYER_TIMEOUT_800_MS 4
#define MOUSE_LAYER_TIMEOUT_NONE 5

/**
 * @brief Process special mouse button keys with dual modifier/mouse
 *        functionality
 *
 * This is the main processing function that implements the dual-function
 * behavior for special mouse button keys. It must be called from the
 * keyboard's process_record_user() function.
 *
 * **Key Processing Logic:**
 *
 * 1. **On ANY key press:** commit every still-undecided held special key to
 *    its modifier role, registering the modifier now so it applies to the key
 *    that triggered this. See confirm_pending_modifiers() below.
 *
 * 2. **On special key press:**
 *    - Initialize state tracking
 *    - Check for external modifiers (modifiers not from our special keys)
 *    - If external mods active: act as a mouse button immediately
 *    - Else if a mouse button is already held (drag in flight): act as a
 *      modifier immediately -- the next event is the drop, not a keypress
 *    - Otherwise: commit to nothing, and emit nothing. The key is a button by
 *      default; the modifier has to be earned before release.
 *
 * 3. **On special key release:**
 *    - If was used with external mods: release mouse button
 *    - Else if converted to mouse (by movement): release mouse button
 *    - Else if used as a modifier (another key, or a scroll): release modifier
 *    - Else nothing ever competed for it: tap the mouse button
 *
 * @param keycode The keycode being processed
 * @param record Pointer to the key event record
 * @return true to continue processing this key, false if key was handled
 *
 * @note This function should be called from process_record_user() like this:
 * @code
 * bool process_record_user(uint16_t keycode, keyrecord_t *record) {
 *     if (!process_special_mouse_keys(keycode, record)) {
 *         return false;
 *     }
 *     // ... other processing
 *     return true;
 * }
 * @endcode
 */
bool process_special_mouse_keys(uint16_t keycode, keyrecord_t *record);

/**
 * @brief Commit any still-undecided held special key to its modifier role
 *
 * Called when some OTHER key is pressed while one of the special keys
 * (MB_SFT, MB_ALT, MB_GUI, MB_CTL) is held but has not yet committed to
 * being either a modifier or a mouse button. Pressing another key is the
 * signal that the held key was meant as a modifier, so its pending mouse
 * button tap must be cancelled.
 *
 * @param keycode The keycode of the OTHER key that was just pressed. Used
 *        only to classify that key (mouse button / special key / ordinary
 *        key), which decides whether mouse mode should be exited on release.
 *
 * @note Idempotent: calling it more than once for the same keypress is
 *       harmless, which is what lets both call sites below invoke it
 *       without coordinating.
 *
 * @note **Two call sites, deliberately.** SM_TD-managed keycodes never make
 *       it to the mouse-key handling: process_smtd() runs FIRST inside
 *       process_record_user() and consumes the record (the function returns
 *       false before process_special_mouse_keys() is reached), and SM_TD
 *       emits its own output through tap_code16()/register_code16(), which
 *       bypass process_record entirely. So an SM_TD key pressed while a
 *       special key was held left that special key looking untouched, and
 *       releasing it fired the "tapped alone" branch -- a phantom mouse
 *       click with no modifiers, seen by the OS as a real click (e.g.
 *       Cmd+Space emitting a stray middle click, which pastes in any app
 *       bound to middle-click-paste). on_smtd_action() therefore calls this
 *       too, for the keys the mouse-key handling will never be given.
 */
void confirm_pending_modifiers(uint16_t keycode);

#endif // QMK_USERSPACE_TOWNK_MOUSE_H
