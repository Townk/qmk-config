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
 * 1. **On ANY key press:** Check all held special keys and mark them as
 *    modifiers if they haven't been used yet. This ensures they act as
 *    modifiers when another key is pressed while they're held.
 *
 * 2. **On special key press:**
 *    - Initialize state tracking
 *    - Check for external modifiers (modifiers not from our special keys)
 *    - If external mods active: Act as mouse button immediately
 *    - Otherwise: Act as modifier (may convert to mouse later)
 *
 * 3. **On special key release:**
 *    - If was used with external mods: Release mouse button
 *    - Else if converted to mouse (by movement): Release mouse button
 *    - Else if used as modifier: Just release modifier
 *    - Else (tapped alone): Release modifier and tap mouse button
 *    - Clean up state and exit mouse mode if needed
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

#endif // QMK_USERSPACE_TOWNK_MOUSE_H
