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

void setup_dynamic_keymap(void) {
    dynamic_keymap_set_key_override(0, &lparen_key_override);
    dynamic_keymap_set_key_override(1, &rparen_key_override);
    dynamic_keymap_set_key_override(2, &exclam_key_override);
}

