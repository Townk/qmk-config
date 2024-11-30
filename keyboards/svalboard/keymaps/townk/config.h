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

#ifndef QMK_USERSPACE_TOWNK_SVALBOARD_CONFIG_H
#define QMK_USERSPACE_TOWNK_SVALBOARD_CONFIG_H

#define VIAL_KEYBOARD_UID {0x1B, 0x18, 0x7D, 0xF2, 0x21, 0xF6, 0x29, 0x48}

// Vial security combos, depending on which unit this is...
#ifdef INIT_EE_HANDS_RIGHT
// right thumb lock
#    define VIAL_UNLOCK_COMBO_ROWS {5, 5}
#    define VIAL_UNLOCK_COMBO_COLS {0, 1}
#elif INIT_EE_HANDS_LEFT
// left thumb lock
#    define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#    define VIAL_UNLOCK_COMBO_COLS {0, 1}
#else
// both thumb locks
#    define VIAL_UNLOCK_COMBO_ROWS {0, 0, 5, 5}
#    define VIAL_UNLOCK_COMBO_COLS {2, 5, 2, 5}
#endif

// Shorten the unlock timeout (needs mod in `quantum/vial.c`; without
// it the override doesn't work)
#define VIAL_UNLOCK_COUNTER_MAX 12

// sm_td
#define MAX_DEFERRED_EXECUTORS 10

#endif  // QMK_USERSPACE_TOWNK_SVALBOARD_CONFIG_H
