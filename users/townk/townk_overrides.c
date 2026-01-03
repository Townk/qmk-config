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

#include "townk_overrides.h"

#include <stdint.h>

#include "dynamic_keymap.h"
#include "quantum_keycodes.h"
#include "vial.h"

const uint8_t custom_ko_options = vial_ko_option_activation_trigger_down | vial_ko_enabled;

const vial_key_override_entry_t lparen_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_LPRN,
    .replacement=KC_AT,
    .layers=~0,
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

const vial_key_override_entry_t rparen_key_override = {
    .trigger_mods=MOD_MASK_SHIFT,
    .trigger=KC_RPRN,
    .replacement=KC_PERC,
    .layers=~0,
    .suppressed_mods=MOD_MASK_SHIFT,
    .negative_mod_mask=0,
    .options=custom_ko_options
};

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

