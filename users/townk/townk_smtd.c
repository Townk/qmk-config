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
#include "action_util.h"
#include "caps_word.h"

#include "keycodes.h"
#include "modifiers.h"
#include "townk_layers.h"
#include "townk_keycodes.h"

#include "sm_td.h"

extern void mouse_mode(bool on);

/**
 * @brief Conditionally disables Caps Word mode based on the keycode.
 *
 * This macro checks if the given keycode should break Caps Word mode.
 * Alphanumeric characters (A-Z, 0-9), minus, underscore, backspace, and delete
 * keys will NOT break Caps Word. Any other key will turn Caps Word off.
 *
 * @param keycode The keycode to check against the allowed list.
 *
 * @note This macro expands to a switch statement and should be used where such
 *       control flow is appropriate.
 */
#define BREAK_CAPS_WORD(keycode) \
    switch ((uint16_t)keycode) { \
        case KC_A ... KC_Z:      \
        case KC_1 ... KC_0:      \
        case KC_MINS:            \
        case KC_BSPC:            \
        case KC_DEL:             \
        case KC_UNDS:            \
            break;               \
        default:                 \
            caps_word_off();     \
    }

/**
 * @brief Performs a custom tap action with additional state management.
 *
 * This macro registers a tap for the specified key, checks if it should break
 * Caps Word mode, and disables mouse mode. It's a convenience wrapper combining
 * three common actions that should occur together on a tap event.
 *
 * @param tap_key The keycode to tap (must be a 16-bit keycode).
 *
 * @note This uses SMTD_TAP_16 from the sm_td library with the first parameter
 *       set to false (no modifiers).
 */
#define CUSTOM_TAP(tap_key)      \
    SMTD_TAP_16(false, tap_key); \
    BREAK_CAPS_WORD(tap_key);    \
    mouse_mode(false)

/**
 * @brief Performs a custom untap (key release) action with state management.
 *
 * This macro unregisters a previously tapped key, checks if it should break
 * Caps Word mode, and disables mouse mode. It's the complementary action to
 * CUSTOM_TAP for releasing a key.
 *
 * @param tap_key The keycode to untap/release (must be a 16-bit keycode).
 *
 * @note This uses SMTD_UNREGISTER_16 from the sm_td library with the first
 *       parameter set to false (no modifiers).
 */
#define CUSTOM_UNTAP(tap_key)           \
    SMTD_UNREGISTER_16(false, tap_key); \
    BREAK_CAPS_WORD(tap_key);           \
    mouse_mode(false)

/**
 * @brief Creates a custom Layer-Tap behavior using the SM_TD dance system.
 *
 * This macro defines a tap-dance behavior where:
 * - Tap: Sends the tap_key using CUSTOM_TAP
 * - Hold: Activates the specified layer and registers the tap_key as held
 * - Release: Restores the previous layer and untaps the key
 * - Double tap and hold: Registers the tap_key as being held down
 *
 * The layer activation is limited to 1 occurrence per hold, and mouse mode
 * is disabled when the layer is activated.
 *
 * @param macro_key The custom keycode that triggers this dance behavior.
 * @param tap_key The keycode to send on tap or register on hold.
 * @param layer The layer to activate when the key is held.
 *
 * @note This creates a complete SMTD_DANCE with proper press/release handling.
 */
#define CUSTOM_LT(macro_key, tap_key, layer)                 \
    SMTD_DANCE(macro_key,                                    \
               NOTHING,                                      \
               CUSTOM_TAP(tap_key),                          \
               SMTD_LIMIT(1,                                 \
                          mouse_mode(false);                 \
                          LAYER_PUSH(layer),                 \
                          SMTD_REGISTER_16(false, tap_key)), \
               SMTD_LIMIT(1,                                 \
                          LAYER_RESTORE(),                   \
                          CUSTOM_UNTAP(tap_key));            \
    )

/**
 * @brief Creates a smart shift behavior with Caps Word integration.
 *
 * This macro defines a tap-dance behavior for an intelligent shift key:
 * - Double tap OR tap while shift is already held: Activates Caps Word mode
 * - Single tap: Sets one-shot shift modifier (next key only)
 * - Hold: Registers as a standard left shift modifier
 * - Release: Unregisters the shift modifier
 *
 * This provides a more ergonomic typing experience by reducing the need to
 * hold shift for capitalization.
 *
 * @param macro_key The custom keycode that triggers this dance behavior.
 *
 * @note Requires NO_ACTION_ONESHOT to NOT be defined for full functionality.
 */
#define SMART_SHIFT(macro_key)                          \
    SMTD_DANCE(macro_key,                               \
               NOTHING,                                 \
               (tap_count > 0 || mods & MOD_MASK_SHIFT) \
                   ? caps_word_on()                     \
                   : set_oneshot_mods(MOD_LSFT),        \
               register_mods(MOD_BIT(KC_LSFT)),         \
               unregister_mods(MOD_BIT(KC_LSFT))        \
    )

/**
 * @brief Conditionally executes different actions based on shift modifier
 *        state.
 *
 * This macro checks if a shift modifier is currently active:
 * - If shift IS active: Unregisters shift, executes normal_action, stores
 *   which shift (left or right) was active in shift_mod variable
 * - If shift is NOT active: Executes shift_action
 *
 * This enables creating keys that behave differently when shift is held, by
 * inverting the shift behavior (shifted becomes unshifted and vice versa).
 *
 * @param normal_action The action to perform when shift is held (after
 *        unregistering shift).
 * @param shift_action The action to perform when shift is not held.
 *
 * @note Requires a static uint8_t shift_mod variable in scope to store which
 *       shift modifier was active.
 */
#define SHIFT_ACTION(normal_action, shift_action)       \
    if (mods & MOD_MASK_SHIFT) {                        \
        shift_mod = mods & MOD_BIT_LSHIFT               \
            ? MOD_BIT_LSHIFT                            \
            : MOD_BIT_RSHIFT;                           \
        unregister_mods(MOD_MASK_SHIFT);                \
        normal_action;                                  \
    } else {                                            \
        shift_action;                                   \
    }

/**
 * @brief Taps different keys based on shift modifier state (inverted
 *        behavior).
 *
 * This macro uses SHIFT_ACTION to create shift-inverted tap behavior:
 * - When shift IS held: Taps normal_key (without shift), then re-registers
 *   shift
 * - When shift is NOT held: Taps shift_key
 *
 * This is useful for creating keys like Backspace/Delete where you want the
 * shifted version to become the unshifted version and vice versa.
 *
 * @param normal_key The keycode to tap when shift is held (sent without
 *        shift).
 * @param shift_key The keycode to tap when shift is not held.
 *
 * @note Requires static uint8_t shift_mod variable in scope.
 */
#define SHIFT_TAP(normal_key, shift_key)         \
    SHIFT_ACTION(SMTD_TAP_16(false, normal_key); \
                 register_mods(shift_mod),       \
                 SMTD_TAP_16(false, shift_key))

/**
 * @brief Registers (holds) different keys based on shift modifier state
 *        (inverted).
 *
 * This macro uses SHIFT_ACTION to create shift-inverted hold behavior:
 * - When shift IS held: Registers normal_key (without shift) and sets
 *   delkey_registered flag
 * - When shift is NOT held: Registers shift_key
 *
 * The delkey_registered flag tracks which path was taken, which is used by
 * SHIFT_UNREGISTER to properly release the correct key.
 *
 * @param normal_key The keycode to register when shift is held (sent without
 *        shift).
 * @param shift_key The keycode to register when shift is not held.
 *
 * @note Requires static bool delkey_registered and static uint8_t shift_mod
 *       variables in scope.
 */
#define SHIFT_REGISTER(normal_key, shift_key)         \
    SHIFT_ACTION(SMTD_REGISTER_16(false, normal_key); \
                 delkey_registered = true,            \
                 SMTD_REGISTER_16(false, shift_key))

/**
 * @brief Unregisters (releases) keys registered by SHIFT_REGISTER.
 *
 * This macro releases the key that was registered by SHIFT_REGISTER:
 * - If delkey_registered is true: Unregisters normal_key, re-registers shift
 *   modifier, and clears the delkey_registered flag
 * - If delkey_registered is false: Unregisters shift_key
 *
 * This ensures proper cleanup and modifier state restoration when releasing
 * a shift-inverted key hold.
 *
 * @param normal_key The keycode that was registered when shift was held.
 * @param shift_key The keycode that was registered when shift was not held.
 *
 * @note Must be paired with SHIFT_REGISTER. Requires static bool
 *       delkey_registered and static uint8_t shift_mod variables in scope.
 */
#define SHIFT_UNREGISTER(normal_key, shift_key)       \
    if (delkey_registered) {                          \
        delkey_registered = false;                    \
        SMTD_UNREGISTER_16(false, normal_key);        \
        register_mods(shift_mod);                     \
    } else {                                          \
        SMTD_UNREGISTER_16(false, shift_key);         \
    }

/**
 * @brief Creates a shift-aware Layer-Tap behavior with inverted shift logic.
 *
 * This macro combines layer-tap functionality with shift-inverted key behavior:
 * - Single tap with shift held: Sends normal_key (without shift), then
 *   re-applies shift
 * - Single tap without shift: Sends shifted_key
 * - Hold: Activates the specified layer and registers the appropriate key
 *   based on shift state
 * - Release: Restores the previous layer and unregisters the held key
 *
 * This is particularly useful for keys like Backspace/Delete where you want:
 * - Normal press: Delete
 * - Shift + press: Backspace
 * - Hold: Activate navigation layer
 *
 * The layer activation is limited to 1 occurrence per hold, and mouse mode
 * is disabled during layer activation and tap actions.
 *
 * @param macro_key The custom keycode that triggers this dance behavior.
 * @param normal_key The keycode to use when shift is held (sent/registered
 *        without shift).
 * @param shifted_key The keycode to use when shift is not held.
 * @param layer The layer to activate when the key is held.
 *
 * @note Requires static bool delkey_registered and static uint8_t shift_mod
 *       variables in scope. Creates a complete SMTD_DANCE with proper state
 *       management.
 */
#define SHIFTED_LT(macro_key, normal_key, shifted_key, layer)        \
    SMTD_DANCE(macro_key,                                            \
               NOTHING,                                              \
               SHIFT_TAP(normal_key, shifted_key);                   \
               mouse_mode(false),                                    \
               SMTD_LIMIT(1,                                         \
                          mouse_mode(false);                         \
                          LAYER_PUSH(layer),                         \
                          SHIFT_REGISTER(normal_key, shifted_key)),  \
               SMTD_LIMIT(1,                                         \
                          LAYER_RESTORE(),                           \
                          SHIFT_UNREGISTER(normal_key, shifted_key); \
                          mouse_mode(false)))

/**
 * @brief SM_TD library callback for handling custom tap-dance behaviors.
 *
 * This function is the main entry point for the SM_TD (State Machine Tap
 * Dance) library. It gets called whenever a custom keycode defined in this
 * implementation is pressed, held, or released. The function dispatches to the
 * appropriate macro handler based on the keycode.
 *
 * The function maintains state across invocations using static variables to
 * support shift-inverted key behaviors and proper modifier state tracking.
 *
 * ### Implemented Keycodes:
 *
 * **CKC_TAB** - Layer-tap with Tab key:
 * - Tap: Sends Tab key
 * - Hold: Activates Symbol layer (_SYM)
 *
 * **CKC_BKTAB** - Layer-tap with Shift+Tab:
 * - Tap: Sends Shift+Tab (back tab)
 * - Hold: Activates Function layer (_FUN)
 *
 * **CKC_SPC** - Layer-tap with Space key:
 * - Tap: Sends Space
 * - Hold: Activates Number layer (_NUM)
 *
 * **CKC_BSPC** - Shift-aware Layer-tap with Delete/Backspace:
 * - Tap (no shift): Sends Delete
 * - Tap (with shift): Sends Backspace
 * - Hold: Activates Navigation layer (_NAV)
 * - This uses inverted shift logic for ergonomic Delete/Backspace access
 *
 * **CKC_SMART_SFT** - Smart Shift key:
 * - Single tap: One-shot shift (next key only)
 * - Double tap OR tap while shift held: Activates Caps Word
 * - Hold: Standard shift modifier
 *
 * @param keycode The 16-bit custom keycode that triggered this action. This
 *        should be one of the custom keycodes defined in the keymap that use
 *        the macros defined in this file.
 *
 * @param action The SM_TD action type indicating what event occurred (press,
 *        release, tap, hold, etc.). This is used internally by the SMTD_DANCE
 *        macros to determine which behavior to execute.
 *
 * @param tap_count The number of times the key has been tapped in quick
 *        succession. Used by behaviors like SMART_SHIFT to differentiate
 *        between single tap and double tap actions.
 *
 * @return smtd_resolution Returns SMTD_RESOLUTION_UNHANDLED to indicate that
 *         if the keycode wasn't handled by any of the defined macros, it
 *         should be processed by other handlers or treated as a standard
 *         keycode. The individual macro expansions may return different
 *         resolution values internally.
 *
 * @note **Static Variables:**
 *       - `delkey_registered`: Tracks whether the "normal" key was registered
 *         during a shift-inverted hold. Used by SHIFT_REGISTER /
 *         SHIFT_UNREGISTER to ensure proper key release and modifier
 *         restoration.
 *       - `shift_mod`: Stores which shift modifier (left or right) was active
 *         when a shift-inverted key was pressed. Used to restore the correct
 *         shift state after the key is released.
 *
 * @note **Modifier Detection:**
 *       If NO_ACTION_ONESHOT is not defined, the function detects both regular
 *       and one-shot modifiers. Otherwise, only regular modifiers are
 *       detected. This affects the behavior of SMART_SHIFT and shift-inverted
 *       keys.
 *
 * @note **Mouse Mode Integration:**
 *       Many behaviors in this function automatically disable mouse mode to
 *       ensure clean state transitions when switching between typing and
 *       navigation modes.
 *
 * @warning This function must be defined exactly as `on_smtd_action` with this
 *          signature to properly integrate with the SM_TD library. The library
 *          expects this function to exist and will call it for all registered
 *          custom keycodes.
 *
 * @see CUSTOM_LT(), SHIFTED_LT(), SMART_SHIFT() for the macro implementations
 * @see sm_td.h for the SM_TD library interface and types
 */
smtd_resolution on_smtd_action(uint16_t keycode, smtd_action action, uint8_t tap_count) {
    static bool    delkey_registered = false;
    static uint8_t shift_mod         = 0;

#ifndef NO_ACTION_ONESHOT
    const uint8_t mods = get_mods() | get_oneshot_mods();
#else
    const uint8_t mods = get_mods();
#endif // NO_ACTION_ONESHOT

    switch (keycode) {
        CUSTOM_LT(CKC_TAB, KC_TAB, _SYM);
        CUSTOM_LT(CKC_BKTAB, MKC_BKTAB, _FUN);
        CUSTOM_LT(CKC_SPC, KC_SPC, _NUM);
        SHIFTED_LT(CKC_BSPC, KC_DEL, KC_BSPC, _NAV);
        SMART_SHIFT(CKC_SMSFT);
    }

    return SMTD_RESOLUTION_UNHANDLED;
}
