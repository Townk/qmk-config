/* Copyright (C) 2026 Thiago Alves (https://github.com/townk)
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

#ifndef QMK_USERSPACE_TOWNK_SWITCHER_H
#define QMK_USERSPACE_TOWNK_SWITCHER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @file townk_switcher.h
 * @brief The MOD+Tab "switcher chord": each thumb Pad reverses its own Nail.
 *
 * Pressing Tab with Ctrl/Alt/Gui held -- the app-switcher or browser-tab
 * gesture -- arms a chord that lasts exactly as long as one of those
 * modifiers stays held. While armed, the thumb Pad on the hand whose Nail
 * started the chord cycles in the REVERSE direction:
 *
 * - Tab (LT-Nail) armed it  -> LT-Pad taps Shift+Tab instead of Backspace.
 * - Shift+Tab (RT-Nail)     -> RT-Pad taps Tab instead of Space.
 *
 * Trigger-specific on purpose: only the arming hand's Pad changes role. The
 * chord direction is set by the ARMING key's emission and is never
 * re-classified by the Pad's own reversed emission -- otherwise the first
 * backward cycle would disarm the very pad that produced it.
 *
 * Shift alone never arms (it is not a switcher modifier), and one-shot
 * modifiers cannot arm either: they are spent by the Tab itself, so by pad
 * time the MOD is no longer held -- which is the definition of "chord over".
 */

/**
 * @brief Record a Tab-family emission, arming the chord if a MOD is held.
 *
 * Call with the keycode actually emitted (KC_TAB or MKC_BKTAB) whenever a
 * Tab or Shift+Tab goes out through a path that can start a switcher chord:
 * the CKC_TAB / CKC_BKTAB dances in townk_smtd.c, and plain KC_TAB /
 * MKC_BKTAB presses seen by process_record_user(). Any other keycode is
 * ignored, as is any call with no Ctrl/Alt/Gui modifier currently held.
 *
 * Deliberately NOT called for the Pads' own reversed emissions.
 */
void switcher_note_emit(uint16_t keycode);

/**
 * @brief Observe one plain key event; false when the event was consumed.
 *
 * The switcher's seam into process_record_user(), for the chord keys that
 * exist as PLAIN keycodes rather than SM_TD dances. On a press it:
 *
 * - reports KC_TAB / MKC_BKTAB / KC_GRV to the chord tracker (arming, when
 *   a switcher MOD is held) and lets the key through -- the arming press
 *   itself must still reach the host;
 * - turns KC_B into a tap of MKC_BKGRV while the MOD+Grave chord is armed,
 *   consuming the event -- macOS's Cmd+Grave window cycle runs backwards
 *   with Cmd+Shift+Grave, and B is L2-East, one key east of Grave.
 *
 * Releases always pass through: a B pressed before the chord armed is a
 * real, registered key whose release must not be eaten, and the release of
 * a consumed press unregisters nothing, which QMK treats as a no-op.
 *
 * @return false when the event was fully handled here, true otherwise.
 */
bool switcher_process_record(uint16_t keycode, bool pressed);

/**
 * @brief True while the LEFT pad should tap Shift+Tab instead of Backspace.
 *
 * Armed by a forward Tab, and only while a Ctrl/Alt/Gui modifier is still
 * held -- the modifier check is repeated here so a just-released MOD cannot
 * win a race against the housekeeping disarm.
 */
bool switcher_backtab_ready(void);

/**
 * @brief True while the RIGHT pad should tap Tab instead of Space.
 *
 * The mirror of switcher_backtab_ready(), armed by a Shift+Tab emission.
 */
bool switcher_tab_ready(void);

/**
 * @brief Disarm the chord once no Ctrl/Alt/Gui modifier remains held.
 *
 * Runs from housekeeping_task_user() every main-loop pass, so the release is
 * observed no matter where the modifier came from -- a real mod key, an
 * SM_TD hold, or the MB_* mouse-layer engine.
 */
void switcher_task(void);

/**
 * @brief Forget everything. Test seam; also safe wherever a hard reset of
 *        the chord state is wanted.
 */
void switcher_reset(void);

#endif // QMK_USERSPACE_TOWNK_SWITCHER_H
