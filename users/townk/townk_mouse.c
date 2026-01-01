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

#include "townk_keycodes.h"
#include "townk_mouse.h"

extern void mouse_mode(bool on);

// ============================================
// Special Mouse Keys Implementation
// ============================================
typedef struct {
    bool is_held;
    bool used_as_modifier;
    bool converted_to_mouse;
    bool mods_on_press;
    bool should_exit_mouse_mode;
} mb_state_t;

static mb_state_t mb_states[4] = {0};

static int get_mb_index(uint16_t keycode) {
    switch (keycode) {
        case MB_SFT: return 0;
        case MB_ALT: return 1;
        case MB_GUI: return 2;
        case MB_CTL: return 3;
        default:     return -1;
    }
}

static uint8_t get_modifier(int index) {
    switch (index) {
        case 0: return MOD_BIT(KC_LSFT);
        case 1: return MOD_BIT(KC_LALT);
        case 2: return MOD_BIT(KC_LGUI);
        case 3: return MOD_BIT(KC_LCTL);
        default: return 0;
    }
}

static uint16_t get_mouse_button(int index) {
    switch (index) {
        case 0: return KC_BTN1;
        case 1: return KC_BTN2;
        case 2: return KC_BTN3;
        case 3: return KC_BTN4;
        default: return KC_NO;
    }
}

static bool is_mouse_button(uint16_t keycode) {
    return keycode >= KC_BTN1 && keycode <= KC_BTN8;
}

bool process_special_mouse_keys(uint16_t keycode, keyrecord_t *record) {
    int mb_index = get_mb_index(keycode);
    bool is_special_key = (mb_index >= 0);
    bool is_mouse_btn = is_mouse_button(keycode);

    // FIRST: For ANY key press, confirm pending special keys as modifiers
    if (record->event.pressed) {
        for (int i = 0; i < 4; i++) {
            if (i == mb_index) continue;

            mb_state_t *state = &mb_states[i];

            if (state->is_held && !state->used_as_modifier && !state->converted_to_mouse && !state->mods_on_press) {
                state->used_as_modifier = true;

                // Defer mouse_mode(false) until release, only for non-mouse keys
                if (!is_mouse_btn && !is_special_key) {
                    state->should_exit_mouse_mode = true;
                }
            }
        }
    }

    // THEN: Handle our custom mouse/modifier keys
    if (is_special_key) {
        mb_state_t *state = &mb_states[mb_index];

        if (record->event.pressed) {
            state->is_held = true;
            state->used_as_modifier = false;
            state->converted_to_mouse = false;
            state->mods_on_press = (get_mods() != 0);
            state->should_exit_mouse_mode = false;

            if (state->mods_on_press) {
                register_code(get_mouse_button(mb_index));
            } else {
                register_mods(get_modifier(mb_index));
            }
        } else {
            if (state->mods_on_press) {
                unregister_code(get_mouse_button(mb_index));
            } else if (state->converted_to_mouse) {
                unregister_code(get_mouse_button(mb_index));
            } else if (state->used_as_modifier) {
                unregister_mods(get_modifier(mb_index));
            } else {
                unregister_mods(get_modifier(mb_index));
                tap_code(get_mouse_button(mb_index));
            }

            // Exit mouse mode on release if flagged
            if (state->should_exit_mouse_mode) {
                mouse_mode(false);
            }

            state->is_held = false;
            state->used_as_modifier = false;
            state->converted_to_mouse = false;
            state->should_exit_mouse_mode = false;
        }

        return false;
    }

    return true;
}

// ============================================
// Pointing Device Hook
// ============================================
report_mouse_t pointing_device_task_kb(report_mouse_t report) {
    if (report.x != 0 || report.y != 0) {
        for (int i = 0; i < 4; i++) {
            mb_state_t *state = &mb_states[i];

            if (state->is_held && !state->used_as_modifier && !state->converted_to_mouse && !state->mods_on_press) {
                unregister_mods(get_modifier(i));
                register_code(get_mouse_button(i));
                state->converted_to_mouse = true;
            }
        }
    }

    return pointing_device_task_user(report);
}
