/* Copyright (C) 2026 Thiago Alves (https://github.com/townk)
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
 * @file townk_idle.c
 * @brief LED idle timeout engine. See townk_idle.h.
 */

#include "townk_idle.h"

#ifndef SMTD_UNIT_TEST
/* rgblight_{disable,enable}_noeeprom() and last_input_activity_elapsed().
 * The host fixture declares its own recording stand-ins before including
 * this file, and cannot see quantum.h. */
#    include "quantum.h"
#endif

/** True while this engine has the LEDs dark. Tracking our own transition
 * (rather than reading rgblight state back) keeps the engine from fighting
 * anything else that toggles rgblight, and makes the on/off edges fire
 * exactly once each. */
static bool leds_idled_off = false;

void led_idle_task(void) {
    bool idle = last_input_activity_elapsed() > TOWNK_LED_IDLE_TIMEOUT_MS;

    if (idle == leds_idled_off) {
        return;
    }

    if (idle) {
        rgblight_disable_noeeprom();
    } else {
        rgblight_enable_noeeprom();
    }
    leds_idled_off = idle;
}

void led_idle_reset(void) {
    leds_idled_off = false;
}
