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
 * @file townk_switcher.c
 * @brief State machine for the MOD+Tab switcher chord. See townk_switcher.h.
 *
 * The whole state is one enum: which direction, if any, the current chord
 * was entered with. Arming happens on a Tab-family emission with a switcher
 * modifier held; disarming happens in exactly one place, when no switcher
 * modifier remains held. There is deliberately no arming timestamp and no
 * timeout -- the chord's lifetime IS the modifier hold, nothing else.
 */

#include "action_util.h"
#include "keycodes.h"
#include "modifiers.h"

#include "townk_keycodes.h"
#include "townk_switcher.h"

#ifndef SMTD_UNIT_TEST
/* tap_code16(), for the plain-key reverser. The host fixture's sm_td shim
 * declares it before this file is included, and cannot see quantum.h. */
#    include "quantum.h"
#endif

/** The modifiers that can hold a switcher chord open. Shift is excluded on
 * purpose: bare Shift+Tab is ordinary typing (and this keymap's CKC_BKTAB
 * emits it constantly), not a switcher. */
#define SWITCHER_MODS (MOD_MASK_CTRL | MOD_MASK_ALT | MOD_MASK_GUI)

enum switcher_direction {
    SWITCHER_IDLE,     ///< no chord in flight
    SWITCHER_FORWARD,  ///< entered with Tab; LT-Pad reverses with Shift+Tab
    SWITCHER_BACKWARD, ///< entered with Shift+Tab; RT-Pad reverses with Tab
};

static uint8_t switcher_direction = SWITCHER_IDLE;

/** The MOD+Grave window-cycle chord (macOS Cmd+Grave). One-directional:
 * armed by a Grave emission, reversed by the key one east of Grave (KC_B on
 * L2), which taps Shift+Grave. Tracked separately from the Tab chord so the
 * real macOS flow -- Cmd+Tab to pick an app, then Cmd+Grave to cycle its
 * windows, all under one Cmd hold -- keeps both reversers live at once. */
static bool switcher_grave_armed = false;

/* Real held modifiers only. A one-shot modifier is spent by the Tab that
 * carries it, so by the time a pad could reverse, it is gone -- which is the
 * definition of "chord over" and needs no special case. */
static bool switcher_mods_held(void) {
    return (get_mods() & SWITCHER_MODS) != 0;
}

void switcher_note_emit(uint16_t keycode) {
    if (!switcher_mods_held()) {
        return;
    }

    switch (keycode) {
        case KC_TAB:
            switcher_direction = SWITCHER_FORWARD;
            break;
        case MKC_BKTAB:
            switcher_direction = SWITCHER_BACKWARD;
            break;
        case KC_GRV:
            switcher_grave_armed = true;
            break;
    }
}

bool switcher_backtab_ready(void) {
    return switcher_direction == SWITCHER_FORWARD && switcher_mods_held();
}

bool switcher_tab_ready(void) {
    return switcher_direction == SWITCHER_BACKWARD && switcher_mods_held();
}

bool switcher_process_record(uint16_t keycode, bool pressed) {
    if (!pressed) {
        return true;
    }

    switch (keycode) {
        case KC_TAB:
        case MKC_BKTAB:
        case KC_GRV:
            /* Arming observes; the key itself must still reach the host. */
            switcher_note_emit(keycode);
            return true;

        case KC_B:
            if (switcher_grave_armed && switcher_mods_held()) {
                tap_code16(MKC_BKGRV);
                return false;
            }
            return true;
    }

    return true;
}

void switcher_task(void) {
    if (!switcher_mods_held()) {
        switcher_direction   = SWITCHER_IDLE;
        switcher_grave_armed = false;
    }
}

void switcher_reset(void) {
    switcher_direction    = SWITCHER_IDLE;
    switcher_grave_armed = false;
}

/**
 * @brief QMK main-loop hook; the chord's only disarm point.
 */
void housekeeping_task_user(void) {
    switcher_task();
}
