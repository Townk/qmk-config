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
#include "caps_word.h"

#include "keycodes.h"
#include "townk_layers.h"
#include "townk_keycodes.h"

#include "sm_td.h"

extern void mouse_mode(bool on);

#define BREAK_CAPS_WORD(keycode)     \
    {                                \
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
        }                            \
    }

#define SMTD_CUSTOM_LT(macro_key, tap_key, layer)       \
    case macro_key: {                                   \
        switch (action) {                               \
            case SMTD_ACTION_TOUCH:                     \
                return SMTD_RESOLUTION_UNCERTAIN;       \
            case SMTD_ACTION_TAP:                       \
                SMTD_TAP_16(false, tap_key);            \
                BREAK_CAPS_WORD(tap_key);               \
                mouse_mode(false);                      \
                return SMTD_RESOLUTION_DETERMINED;      \
            case SMTD_ACTION_HOLD:                      \
                if (tap_count == 0) {                   \
                    mouse_mode(false);                  \
                    LAYER_PUSH(layer);                  \
                } else {                                \
                    SMTD_REGISTER_16(false, tap_key);   \
                }                                       \
                return SMTD_RESOLUTION_DETERMINED;      \
            case SMTD_ACTION_RELEASE:                   \
                if (tap_count == 0) {                   \
                    LAYER_RESTORE();                    \
                } else {                                \
                    SMTD_UNREGISTER_16(false, tap_key); \
                    BREAK_CAPS_WORD(tap_key);           \
                    mouse_mode(false);                  \
                }                                       \
                return SMTD_RESOLUTION_DETERMINED;      \
        }                                               \
        return SMTD_RESOLUTION_DETERMINED;              \
    }

/**
 *
 */
smtd_resolution on_backspace_action(smtd_action action, const uint8_t tap_count, const uint8_t mods) {
    static bool    delkey_registered = false;
    static uint8_t shift_mod         = 0;
    switch (action) {
        case SMTD_ACTION_TOUCH:
            return SMTD_RESOLUTION_UNCERTAIN;

        case SMTD_ACTION_TAP:
            if (mods & MOD_MASK_SHIFT) {
                shift_mod = mods & MOD_BIT_LSHIFT ? MOD_BIT_LSHIFT : MOD_BIT_RSHIFT;
                unregister_mods(MOD_MASK_SHIFT);
                SMTD_TAP_16(false, KC_DEL);
                register_mods(shift_mod);
            } else {
                SMTD_TAP_16(false, KC_BSPC);
            }
            mouse_mode(false);
            return SMTD_RESOLUTION_DETERMINED;

        case SMTD_ACTION_HOLD:
            if (tap_count) {
                if (mods & MOD_MASK_SHIFT) {
                    shift_mod = mods & MOD_BIT_LSHIFT ? MOD_BIT_LSHIFT : MOD_BIT_RSHIFT;
                    unregister_mods(MOD_MASK_SHIFT);
                    SMTD_REGISTER_16(false, KC_DEL);
                    delkey_registered = true;
                } else {
                    SMTD_REGISTER_16(false, KC_BSPC);
                }
            } else {
                mouse_mode(false);
                LAYER_PUSH(_NAV);
            }
            return SMTD_RESOLUTION_DETERMINED;

        case SMTD_ACTION_RELEASE:
            if (tap_count) {
                if (delkey_registered) {
                    delkey_registered = false;
                    SMTD_UNREGISTER_16(false, KC_DEL);
                    register_mods(shift_mod);
                } else {
                    SMTD_UNREGISTER_16(false, KC_BSPC);
                }
                mouse_mode(false);
            } else {
                LAYER_RESTORE();
            }
            return SMTD_RESOLUTION_DETERMINED;
    } // end of switch (action)

    return SMTD_RESOLUTION_DETERMINED; // should never reach here anyway
}

smtd_resolution on_smart_shift_action(smtd_action action, const uint8_t tap_count, const uint8_t mods) {
    switch (action) {
        case SMTD_ACTION_TOUCH:
            return SMTD_RESOLUTION_UNCERTAIN;

        case SMTD_ACTION_TAP:
            if (tap_count || mods & MOD_MASK_SHIFT) {
                caps_word_on();
            } else {
                set_oneshot_mods(MOD_LSFT);
            }
            return SMTD_RESOLUTION_DETERMINED;

        case SMTD_ACTION_HOLD:
            register_mods(MOD_BIT(KC_LSFT));
            return SMTD_RESOLUTION_DETERMINED;

        case SMTD_ACTION_RELEASE:
            unregister_mods(MOD_BIT(KC_LSFT));
            return SMTD_RESOLUTION_DETERMINED;
    } // end of switch (KC_BSPC)

    return SMTD_RESOLUTION_DETERMINED; // should never reach here anyway
}

smtd_resolution on_smtd_action(uint16_t keycode, smtd_action action, uint8_t tap_count) {
#ifndef NO_ACTION_ONESHOT
    const uint8_t mods = get_mods() | get_oneshot_mods();
#else
    const uint8_t mods = get_mods();
#endif // NO_ACTION_ONESHOT
    switch (keycode) {
        SMTD_CUSTOM_LT(CKC_TAB, KC_TAB, _SYM);
        SMTD_CUSTOM_LT(CKC_BKTAB, S(KC_TAB), _FUN);
        SMTD_CUSTOM_LT(CKC_SPC, KC_SPC, _NUM);

        case CKC_BSPC:
            return on_backspace_action(action, tap_count, mods);

        case CKC_SMART_SFT:
            return on_smart_shift_action(action, tap_count, mods);
    }

    return SMTD_RESOLUTION_UNHANDLED;
}
