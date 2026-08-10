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
 * @file townk_mods.c
 * @brief Reference-counted modifier ownership -- see townk_mods.h
 *
 * @author Thiago Alves
 */

#include "action.h" // register_mods / unregister_mods
#include "townk_mods.h"

/** One claim counter per modifier bit. QMK modifier masks are 8 bits. */
#define MOD_BIT_COUNT 8

/** @private */
static uint8_t claims[MOD_BIT_COUNT] = {0};

void mods_acquire(uint8_t mods) {
    for (uint8_t bit = 0; bit < MOD_BIT_COUNT; bit++) {
        uint8_t mask = (uint8_t)(1 << bit);
        if (!(mods & mask)) continue;

        // Register on the FIRST claim only; further claims just count.
        if (claims[bit] == 0) {
            register_mods(mask);
        }

        // Saturate rather than wrap. An overflow here would mean thousands of
        // unbalanced claims, and dropping one is far better than wrapping to
        // zero and releasing a modifier somebody still holds.
        if (claims[bit] < UINT8_MAX) {
            claims[bit]++;
        }
    }
}

void mods_release(uint8_t mods) {
    for (uint8_t bit = 0; bit < MOD_BIT_COUNT; bit++) {
        uint8_t mask = (uint8_t)(1 << bit);
        if (!(mods & mask)) continue;

        // Releasing something never claimed is a caller bug, but silently
        // ignoring it beats unregistering a modifier held by someone else.
        if (claims[bit] == 0) continue;

        claims[bit]--;
        if (claims[bit] == 0) {
            unregister_mods(mask);
        }
    }
}

uint8_t mods_claim_count(uint8_t mod_bit) {
    for (uint8_t bit = 0; bit < MOD_BIT_COUNT; bit++) {
        if (mod_bit & (uint8_t)(1 << bit)) {
            return claims[bit];
        }
    }

    return 0;
}

void mods_reset(void) {
    for (uint8_t bit = 0; bit < MOD_BIT_COUNT; bit++) {
        if (claims[bit] > 0) {
            unregister_mods((uint8_t)(1 << bit));
        }
        claims[bit] = 0;
    }
}
