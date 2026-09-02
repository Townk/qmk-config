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

#ifndef QMK_USERSPACE_TOWNK_IDLE_H
#define QMK_USERSPACE_TOWNK_IDLE_H

/**
 * @file townk_idle.h
 * @brief Turn the LEDs off after an idle timeout; back on at the first touch.
 *
 * The keyboard's LEDs otherwise burn all night unless the board is
 * unplugged -- and that nightly unplug is what kept re-triggering the OS
 * misdetection saga. One task function, run from housekeeping_task_user()
 * every scan pass, compares QMK's last_input_activity_elapsed() -- which
 * already timestamps every key press, trackball motion, and scroll, from
 * either half -- against TOWNK_LED_IDLE_TIMEOUT_MS:
 *
 * - idle past the timeout: rgblight_disable_noeeprom(), once;
 * - any input after that: rgblight_enable_noeeprom(), on the next
 *   housekeeping pass (~1ms), before the user can look down.
 *
 * The _noeeprom calls never persist, so a power-up always starts lit, and
 * all color state (active-layer color, caps-word layer) keeps updating
 * while dark -- waking shows the CURRENT colors, not stale ones.
 */

/**
 * @brief Idle threshold in milliseconds before the LEDs go dark.
 *
 * Overridable from the keymap's config.h; the default matches "away from
 * the desk", not "reading the screen".
 */
#ifndef TOWNK_LED_IDLE_TIMEOUT_MS
#    define TOWNK_LED_IDLE_TIMEOUT_MS 60000
#endif

/**
 * @brief The whole engine: evaluate idleness, cross the on/off transitions.
 *
 * Call from housekeeping_task_user(). Idempotent between transitions --
 * running it every scan pass causes no LED churn.
 */
void led_idle_task(void);

/**
 * @brief Forget the idled-off state. Test seam.
 */
void led_idle_reset(void);

#endif // QMK_USERSPACE_TOWNK_IDLE_H
