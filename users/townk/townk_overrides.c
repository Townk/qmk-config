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
 * @file townk_overrides.c
 * @brief Key override definitions for custom key behavior with modifiers
 *
 * This file implements key overrides using the Vial firmware's key override
 * system. Key overrides allow keys to produce different outputs when pressed
 * with specific modifier keys, without affecting the base keymap.
 *
 * **Current Overrides:**
 * - Shift + `(` produces `@`
 * - Shift + `)` produces `%`
 * - Shift + `!` produces `^`
 *
 * These overrides are active on all layers and suppress the shift modifier
 * when triggered, so the replacement key is sent without shift.
 *
 * @author Thiago Alves
 * @date 2024
 */

#include "townk_overrides.h"

#include <stdint.h>

#include "dynamic_keymap.h"
#include "quantum_keycodes.h"
#include "townk_layers.h"
#include "vial.h"

/**
 * @brief Default options for all key overrides
 *
 * This constant defines the common behavior options for key overrides in this
 * configuration:
 *
 * - `vial_ko_option_activation_trigger_down`: Activate on key press (down).
 * - `vial_ko_enabled`: Key override is enabled.
 *
 * These options are applied to all key override entries to ensure consistent
 * behavior across all overrides.
 */
const uint8_t custom_ko_options = vial_ko_option_activation_trigger_down | vial_ko_enabled;

/**
 * @brief Key override: Shift + Left Parenthesis → At Sign
 *
 * Transforms `Shift + (` into `@` on all layers.
 *
 * **Configuration:**
 * - Trigger: Left parenthesis key with Shift modifier
 * - Replacement: At sign (`@`)
 * - Layers: All layers (`~0` = all bits set)
 * - Suppressed mods: Shift (prevents `Shift + @`)
 * - Result: Pressing `Shift + (` produces `@` without shift
 */
const vial_key_override_entry_t lparen_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_LPRN,
    .replacement=KC_AT,
    .layers=~0,
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

/**
 * @brief Key override: Shift + Right Parenthesis → Percent Sign
 *
 * Transforms `Shift + )` into `%` on all layers.
 *
 * **Configuration:**
 * - Trigger: Right parenthesis key with Shift modifier
 * - Replacement: Percent sign (`%`)
 * - Layers: All layers (`~0` = all bits set)
 * - Suppressed mods: Shift (prevents `Shift + %`)
 * - Result: Pressing `Shift + )` produces `%` without shift
 */
const vial_key_override_entry_t rparen_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_RPRN,
    .replacement=KC_PERC,
    .layers=~0,
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

/**
 * @brief Key override: Shift + Exclamation Mark → Caret/Circumflex
 *
 * Transforms `Shift + !` into `^` on all layers.
 *
 * **Configuration:**
 * - Trigger: Exclamation mark key with Shift modifier
 * - Replacement: Caret/circumflex (`^`)
 * - Layers: All layers (`~0` = all bits set)
 * - Suppressed mods: Shift (prevents `Shift + ^`)
 * - Result: Pressing `Shift + !` produces `^` without shift
 */
const vial_key_override_entry_t exclam_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_EXLM,
    .replacement=KC_CIRC,
    .layers=~0,
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

/**
 * @brief Key override: Shift + 1 → 5
 *
 * Transforms `Shift + 1` into `5` on Game layer.
 *
 * **Configuration:**
 * - Trigger: Left parenthesis key with Shift modifier
 * - Replacement: 5
 * - Layers: Game
 * - Suppressed mods: Shift (prevents `Shift + 5`)
 * - Result: Pressing `Shift + 1` produces `5` without shift
 */
const vial_key_override_entry_t game_1_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_1,
    .replacement=KC_5,
    .layers=(1 << _GAME),
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

/**
 * @brief Key override: Shift + 2 → 6
 *
 * Transforms `Shift + 2` into `6` on Game layer.
 *
 * **Configuration:**
 * - Trigger: Left parenthesis key with Shift modifier
 * - Replacement: 6
 * - Layers: Game
 * - Suppressed mods: Shift (prevents `Shift + 6`)
 * - Result: Pressing `Shift + 2` produces `6` without shift
 */
const vial_key_override_entry_t game_2_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_2,
    .replacement=KC_6,
    .layers=(1 << _GAME),
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

/**
 * @brief Key override: Shift + 3 → 7
 *
 * Transforms `Shift + 3` into `7` on Game layer.
 *
 * **Configuration:**
 * - Trigger: Left parenthesis key with Shift modifier
 * - Replacement: 7
 * - Layers: Game
 * - Suppressed mods: Shift (prevents `Shift + 7`)
 * - Result: Pressing `Shift + 3` produces `7` without shift
 */
const vial_key_override_entry_t game_3_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_3,
    .replacement=KC_7,
    .layers=(1 << _GAME),
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

/**
 * @brief Key override: Shift + 4 → 8
 *
 * Transforms `Shift + 4` into `8` on Game layer.
 *
 * **Configuration:**
 * - Trigger: Left parenthesis key with Shift modifier
 * - Replacement: 8
 * - Layers: Game
 * - Suppressed mods: Shift (prevents `Shift + 8`)
 * - Result: Pressing `Shift + 4` produces `8` without shift
 */
const vial_key_override_entry_t game_4_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_4,
    .replacement=KC_8,
    .layers=(1 << _GAME),
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

/**
 * @brief Key override: Shift + 0 → 9
 *
 * Transforms `Shift + 0` into `9` on Game layer.
 *
 * **Configuration:**
 * - Trigger: Left parenthesis key with Shift modifier
 * - Replacement: 9
 * - Layers: Game
 * - Suppressed mods: Shift (prevents `Shift + 9`)
 * - Result: Pressing `Shift + 0` produces `9` without shift
 */
const vial_key_override_entry_t game_0_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_0,
    .replacement=KC_9,
    .layers=(1 << _GAME),
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

void setup_dynamic_keymap(void) {
    dynamic_keymap_set_key_override(0, &lparen_key_override);
    dynamic_keymap_set_key_override(1, &rparen_key_override);
    dynamic_keymap_set_key_override(2, &exclam_key_override);
    dynamic_keymap_set_key_override(3, &game_0_key_override);
    dynamic_keymap_set_key_override(4, &game_1_key_override);
    dynamic_keymap_set_key_override(5, &game_2_key_override);
    dynamic_keymap_set_key_override(6, &game_3_key_override);
    dynamic_keymap_set_key_override(7, &game_4_key_override);
}

