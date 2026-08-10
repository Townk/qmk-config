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
 * @file townk_mods.h
 * @brief Reference-counted modifier ownership
 *
 * More than one thing in this userspace can want the same modifier held at
 * the same time — MB_ALT acting as Alt while a held key also contributes Alt
 * to a click, for instance. Whoever finishes first must not switch the
 * modifier off underneath whoever is still using it.
 *
 * Claim with mods_acquire(), give up with mods_release(). The modifier is
 * registered when its first claim arrives and unregistered only when the last
 * one goes away. Calls must be paired; the mask passed to release should match
 * the one passed to acquire.
 *
 * This exists because the alternative — each caller doing its own
 * register_mods()/unregister_mods() — is correct only while exactly one thing
 * can ever own a given modifier. That was true until keys could contribute
 * modifiers to clicks, and it fails silently rather than loudly: the modifier
 * simply stops being held mid-gesture.
 *
 * @author Thiago Alves
 */

#ifndef QMK_USERSPACE_TOWNK_MODS_H
#define QMK_USERSPACE_TOWNK_MODS_H

#include <stdint.h>

/**
 * @brief Claim one or more modifiers, registering any not already held
 * @param mods MOD_BIT mask of the modifiers to claim
 */
void mods_acquire(uint8_t mods);

/**
 * @brief Give up a claim, unregistering only modifiers nobody else wants
 * @param mods MOD_BIT mask matching an earlier mods_acquire()
 */
void mods_release(uint8_t mods);

/**
 * @brief Number of outstanding claims on a single modifier
 *
 * For tests and debugging; production code should not need to ask.
 *
 * @param mod_bit A single MOD_BIT value
 * @return Outstanding claim count, 0 if the modifier is not held
 */
uint8_t mods_claim_count(uint8_t mod_bit);

/**
 * @brief Drop every claim and unregister everything held
 *
 * A panic button for tests and for recovering from a state that should not
 * happen; not part of normal operation.
 */
void mods_reset(void);

#endif // QMK_USERSPACE_TOWNK_MODS_H
