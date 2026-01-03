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

#ifndef QMK_USERSPACE_TOWNK_OVERRIDE_H
#define QMK_USERSPACE_TOWNK_OVERRIDE_H

/**
 * @brief Register all key overrides with the dynamic keymap system
 *
 * This function registers all defined key overrides with Vial's dynamic keymap
 * system. It must be called during keyboard initialization (typically from
 * keyboard_post_init_user()) to activate the key override functionality.
 *
 * **Registered Overrides:**
 * 1. Slot 0: Left parenthesis override (`Shift + (` → `@`)
 * 2. Slot 1: Right parenthesis override (`Shift + )` → `%`)
 * 3. Slot 2: Exclamation mark override (`Shift + !` → `^`)
 *
 * After this function is called, the key overrides will be active and will
 * transform the specified key combinations according to their definitions.
 *
 * @note This function should be called once during keyboard initialization
 *
 * @see keyboard_post_init_user() in keymap.c where this function is called
 * @see lparen_key_override, rparen_key_override, exclam_key_override for
 *      override definitions
 */
void setup_dynamic_keymap(void);

#endif // QMK_USERSPACE_TOWNK_OVERRIDE_H

