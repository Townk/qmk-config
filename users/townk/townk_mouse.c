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

/**
 * @file townk_mouse.c
 * @brief Special mouse button implementation with dual-function modifier/mouse
 *        keys
 *
 * On the mouse layer, MB_SFT / MB_ALT / MB_GUI / MB_CTL are each both a mouse
 * button and a keyboard modifier. Which one a given press turns out to be is
 * decided by what happens next.
 *
 * **The key is a BUTTON first.** A press commits to nothing and emits nothing.
 * The modifier is the exceptional role and must be EARNED by a competing
 * signal arriving before release. Nothing happening is not a signal -- it is
 * the default, and the default is a click.
 *
 * **Behavior Rules** -- while a press is still undecided:
 *
 * | What happens next            | Role it takes | What is emitted          |
 * |------------------------------|---------------|--------------------------|
 * | Released, nothing else       | button        | a click, at any duration |
 * | Pointer moves                | button, held  | button down -> drag      |
 * | Scroll                       | modifier      | keeps Cmd+scroll as zoom |
 * | Another key is PRESSED       | modifier      | applies to that key      |
 * | Another MB_* key is pressed  | modifier      | giving e.g. Cmd+click    |
 *
 * External modifiers already held at press time are decided immediately: the
 * key is a mouse button, so Shift+click and friends work.
 *
 * Two properties of that table are load-bearing, and both were learned the
 * hard way:
 *
 * 1. Resolution happens on the other key's PRESS, never its release -- by
 *    release the key has already been sent, and the modifier would be too
 *    late to apply to it.
 *
 * 2. The button role is the DEFAULT rather than a fallback. The previous
 *    design registered the modifier on press and emitted the button at
 *    release only if nothing had contradicted it -- which meant proving a
 *    negative from observed keypresses, and it could not observe the ones
 *    SM_TD consumed upstream of process_record_user(). Every press of
 *    Cmd+Space therefore emitted a real, modifier-less middle click; it only
 *    looked intermittent because it was invisible unless the pointer happened
 *    to rest over something that reacts to one. Scrolling with a key held had
 *    the same defect. Under these rules an unobserved keypress can no longer
 *    manufacture a click, because a click requires the absence of every
 *    signal rather than the absence of the ones we managed to see.
 *
 * The rules are executable: tests/test_townk_mouse.py drives this file on the
 * host and asserts the table above, one test per row.
 *
 * @author Thiago Alves
 * @date 2024
 */

#include "townk_keycodes.h"
#include "townk_mouse.h"

/**
 * @brief External function to toggle mouse mode on/off
 * @param on True to enable mouse mode, false to disable
 */
extern void mouse_mode(bool on);

/**
 * @brief State information for a mouse button key
 *
 * This structure tracks the current state and behavior mode of each special
 * mouse button key (MB_SFT, MB_ALT, MB_GUI, MB_CTL) to determine whether it
 * should act as a modifier or mouse button.
 */
typedef struct {
    bool is_held;                  ///< True if the key is currently pressed down
    bool used_as_modifier;         ///< True if the key has been used as a modifier in this press
    bool converted_to_mouse;       ///< True if the key was converted to mouse button due to mouse movement
    bool mods_on_press;            ///< True if external modifiers were active when this key was pressed
    bool should_exit_mouse_mode;   ///< True if mouse mode should be exited when this key is released
} mb_state_t;

/**
 * @brief State array for all four special mouse button keys
 *
 * Index mapping:
 * - 0: MB_SFT (Shift)
 * - 1: MB_ALT (Alt)
 * - 2: MB_GUI (GUI/Super/Command)
 * - 3: MB_CTL (Control)
 */
static mb_state_t mb_states[4] = {0};

/**
 * @brief Maps a special mouse button keycode to its state array index
 *
 * @param keycode The keycode to map (MB_SFT, MB_ALT, MB_GUI, or MB_CTL)
 * @return int Index into mb_states array (0-3), or -1 if keycode is not a
 *         special mouse button
 */
static int get_mb_index(uint16_t keycode) {
    switch (keycode) {
        case MB_SFT: return 0;
        case MB_ALT: return 1;
        case MB_GUI: return 2;
        case MB_CTL: return 3;
        default:     return -1;
    }
}

/**
 * @brief Gets the modifier bit corresponding to a state array index
 *
 * @param index Index into mb_states array (0-3)
 * @return uint8_t Modifier bit mask (MOD_BIT format), or 0 if index is invalid
 */
static uint8_t get_modifier(int index) {
    switch (index) {
        case 0: return MOD_BIT(KC_LSFT);
        case 1: return MOD_BIT(KC_LALT);
        case 2: return MOD_BIT(KC_LGUI);
        case 3: return MOD_BIT(KC_LCTL);
        default: return 0;
    }
}

/**
 * @brief Gets the mouse button keycode corresponding to a state array index
 *
 * @param index Index into mb_states array (0-3)
 * @return uint16_t Mouse button keycode (KC_BTN1-KC_BTN4), or KC_NO if index
 *         is invalid
 */
static uint16_t get_mouse_button(int index) {
    switch (index) {
        case 0: return KC_BTN1;
        case 1: return KC_BTN2;
        case 2: return KC_BTN3;
        case 3: return KC_BTN4;
        default: return KC_NO;
    }
}

/**
 * @brief Checks if a keycode represents a standard mouse button
 *
 * @param keycode The keycode to check
 * @return true If keycode is a mouse button (KC_BTN1 through KC_BTN8)
 * @return false Otherwise
 */
static bool is_mouse_button(uint16_t keycode) {
    return keycode >= KC_BTN1 && keycode <= KC_BTN8;
}

void confirm_pending_modifiers(uint16_t keycode) {
    int  mb_index       = get_mb_index(keycode);
    bool is_special_key = (mb_index >= 0);
    bool is_mouse_btn   = is_mouse_button(keycode);

    for (int i = 0; i < 4; i++) {
        // Skip the key being pressed if it's also a special key
        if (i == mb_index) continue;

        mb_state_t *state = &mb_states[i];

        // If a special key is held but hasn't committed to a role yet,
        // make it a modifier
        if (state->is_held && !state->used_as_modifier && !state->converted_to_mouse && !state->mods_on_press) {
            state->used_as_modifier = true;

            // Register the modifier HERE, not at press time. Every caller
            // reaches this before the key that triggered it is emitted --
            // process_record_user() returns true afterwards, and
            // on_smtd_action() runs this ahead of its own switch -- so the
            // modifier is down in time to apply to that key.
            register_mods(get_modifier(i));

            // Defer mouse_mode(false) until release, only for non-mouse
            // keys
            if (!is_mouse_btn && !is_special_key) {
                state->should_exit_mouse_mode = true;
            }
        }
    }
}

bool process_special_mouse_keys(uint16_t keycode, keyrecord_t *record) {
    int mb_index = get_mb_index(keycode);
    bool is_special_key = (mb_index >= 0);

    // FIRST: For ANY key press, confirm pending special keys as modifiers
    if (record->event.pressed) {
        confirm_pending_modifiers(keycode);
    }

    // THEN: Handle our custom mouse/modifier keys
    if (is_special_key) {
        mb_state_t *state = &mb_states[mb_index];

        if (record->event.pressed) {
            // Initialize state for this key press
            state->is_held = true;
            state->used_as_modifier = false;
            state->converted_to_mouse = false;

            // Check if there are external modifiers (not from our special
            // keys)
            // Check all modifier sources: normal, oneshot, and weak mods
            uint8_t current_mods = get_mods() | get_oneshot_mods() | get_weak_mods();
            uint8_t our_mods = 0;
            for (int i = 0; i < 4; i++) {
                // Skip the key being pressed to avoid false detection
                if (i == mb_index) continue;

                if (mb_states[i].is_held && !mb_states[i].mods_on_press) {
                    our_mods |= get_modifier(i);
                }
            }
            state->mods_on_press = ((current_mods & ~our_mods) != 0);
            state->should_exit_mouse_mode = false;

            // If external modifiers are active, act as mouse button
            if (state->mods_on_press) {
                register_code(get_mouse_button(mb_index));
            }
            // Otherwise commit to NOTHING yet. On this layer the key is a
            // button by default and a modifier only by exception, so the
            // modifier has to be earned by something happening next --
            // another key, or a scroll. Claiming it up-front is what made
            // the old design have to prove a negative at release time.
        } else {
            // Key release - determine what to release based on how the key was
            // used
            if (state->mods_on_press) {
                // Was used as mouse button due to external modifiers
                unregister_code(get_mouse_button(mb_index));
            } else if (state->converted_to_mouse) {
                // Was converted to mouse button by mouse movement
                unregister_code(get_mouse_button(mb_index));
            } else if (state->used_as_modifier) {
                // Was used as a modifier (another key was pressed, or a scroll)
                unregister_mods(get_modifier(mb_index));
            } else {
                // Nothing ever competed for this press, so it keeps its
                // default role: a click. No modifier to release first --
                // none was ever registered.
                tap_code(get_mouse_button(mb_index));
            }

            // Exit mouse mode on release if flagged
            if (state->should_exit_mouse_mode) {
                mouse_mode(false);
            }

            // Reset state
            state->is_held = false;
            state->used_as_modifier = false;
            state->converted_to_mouse = false;
            state->should_exit_mouse_mode = false;
        }

        return false; // Key was handled, stop further processing
    }

    return true; // Not a special key, continue processing
}

/**
 * @brief QMK pointing device task hook for automatic modifier-to-mouse
 *        conversion
 *
 * This function is called by QMK on every pointing device report (mouse
 * movement). It implements the automatic conversion of held special keys from
 * modifiers to mouse buttons when the mouse/trackball/trackpad moves.
 *
 * **Conversion Logic:**
 * When mouse movement is detected (x or y != 0), this function checks each
 * special key to see if it should be converted to a mouse button. A key is
 * converted if:
 * - The key is currently held (is_held)
 * - It hasn't been used as a modifier yet (used_as_modifier is false)
 * - It hasn't already been converted (converted_to_mouse is false)
 * - No external modifiers were active when pressed (mods_on_press is false)
 *
 * When conversion happens:
 * 1. The modifier is unregistered
 * 2. The corresponding mouse button is registered
 * 3. The state is marked as converted_to_mouse
 *
 * This allows users to hold a special key and then move the mouse to perform
 * drag operations without having to press multiple keys.
 *
 * @param report The mouse movement report from QMK
 * @return report_mouse_t The report to be sent (passed to user layer)
 *
 * @note This is a QMK keyboard-level hook. The report is passed to
 *       pointing_device_task_user() for further user-level processing.
 */
report_mouse_t pointing_device_task_kb(report_mouse_t report) {
    bool moved      = (report.x != 0 || report.y != 0);
    bool scrolled   = (report.h != 0 || report.v != 0);

    // Pointer motion and scrolling resolve an undecided key in OPPOSITE
    // directions, so motion is checked first and wins when a report carries
    // both (a drag with a little scroll noise is still a drag).
    if (moved || scrolled) {
        for (int i = 0; i < 4; i++) {
            mb_state_t *state = &mb_states[i];

            if (!state->is_held || state->used_as_modifier || state->converted_to_mouse || state->mods_on_press) {
                continue;
            }

            if (moved) {
                // Dragging: hold the button down for the whole gesture.
                register_code(get_mouse_button(i));
                state->converted_to_mouse = true;
            } else {
                // Scrolling: the key is qualifying the scroll, not clicking
                // through it -- this is what keeps Cmd+scroll as zoom. It
                // also stops the release from firing a stray click, which
                // is the same defect as a modifier-less phantom press.
                register_mods(get_modifier(i));
                state->used_as_modifier = true;
            }
        }
    }

    return pointing_device_task_user(report);
}
