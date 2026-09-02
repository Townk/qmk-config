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

#ifndef QMK_USERSPACE_TOWNK_SVALBOARD_CONFIG_H
#define QMK_USERSPACE_TOWNK_SVALBOARD_CONFIG_H

#define VIAL_KEYBOARD_UID {0x1B, 0x18, 0x7D, 0xF2, 0x21, 0xF6, 0x29, 0x48}

// Shorten the unlock timeout (needs mod in `quantum/vial.c`; without
// it the override doesn't work)
#define VIAL_UNLOCK_COUNTER_MAX 12

// sm_td
#define SMTD_GLOBAL_SEQUENCE_TERM 100
#define SMTD_GLOBAL_RELEASE_TERM 15

// Turn the LEDs off after this much input silence (keys, trackballs,
// scroll -- either half) and back on at the first touch; see townk_idle.h.
// Tune freely: 60000 = 1 minute, 120000 = 2 minutes.
#define TOWNK_LED_IDLE_TIMEOUT_MS 60000

// Report the detected host OS once per boot and never again. Without this,
// every later string-descriptor read on the bus keeps feeding the detector,
// and a single mid-session read with wLength 0x04 flips the guess from
// OS_MACOS to OS_WINDOWS (the Windows branch is matched first). Svalboard's
// process_detected_host_os_kb() then drops the scroll divisor from 120 back
// to 1, making the scroll trackball ~120x more sensitive until the next
// reset. Host switches still re-detect: OS_DETECTION_KEYBOARD_RESET reboots
// the board on USB re-enumeration, which clears the one-shot latch.
#define OS_DETECTION_SINGLE_REPORT

#endif  // QMK_USERSPACE_TOWNK_SVALBOARD_CONFIG_H
