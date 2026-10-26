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

#include "townk_announce.h"

#include <stdio.h>
#include <string.h>

#define VIA_CUSTOM_GET_VALUE 0x08  // id_custom_get_value

static bool             subscribed  = false;
static bool             full_needed = false;
static uint32_t         lease_until = 0;
static announce_state_t last;

bool announce_handle_command(uint8_t *data, uint8_t length, uint32_t now) {
    if (length < 9 || data[1] != ANNOUNCE_CHANNEL) {
        return false;
    }
    if (data[0] != VIA_CUSTOM_GET_VALUE || data[2] != ANNOUNCE_VALUE_SUBSCRIBE) {
        // Our channel, but not a command this version knows: answer
        // "unhandled" rather than let Svalboard's handler read it as one
        // of its own value ids.
        data[0] = 0xFF;  // id_unhandled
        return true;
    }

    uint16_t lease = data[3] | (uint16_t)(data[4] << 8);
    if (lease < ANNOUNCE_LEASE_MIN_MS) lease = ANNOUNCE_LEASE_MIN_MS;
    if (lease > ANNOUNCE_LEASE_MAX_MS) lease = ANNOUNCE_LEASE_MAX_MS;

    subscribed  = true;
    full_needed = true;  // every renewal re-announces everything
    lease_until = now + lease;

    data[3] = 's';
    data[4] = 'v';
    data[5] = 'w';
    data[6] = ANNOUNCE_VERSION;
    data[7] = lease & 0xFF;
    data[8] = lease >> 8;
    return true;
}

bool announce_subscribed(void) {
    return subscribed;
}

void announce_reset(void) {
    subscribed  = false;
    full_needed = false;
}

size_t announce_poll(const announce_state_t *state, uint32_t now, char *buf, size_t size) {
    if (!subscribed) {
        return 0;
    }
    if ((int32_t)(now - lease_until) >= 0) {
        subscribed = false;
        return 0;
    }

    bool layer_changed = full_needed || state->active != last.active || state->default_layers != last.default_layers || state->mods != last.mods || state->status != last.status;
    bool rows_changed  = full_needed || memcmp(state->rows, last.rows, sizeof(last.rows)) != 0;
    full_needed        = false;
    last               = *state;

    size_t n = 0;
    if (layer_changed && n < size) {
        n += snprintf(buf + n, size - n, "svw L %08lX %08lX %02X %02X\n", (unsigned long)state->active, (unsigned long)state->default_layers, state->mods, state->status);
    }
    if (rows_changed && n + 7 < size) {
        n += snprintf(buf + n, size - n, "svw K ");
        for (int r = 0; r < ANNOUNCE_ROWS && n + 3 < size; r++) {
            n += snprintf(buf + n, size - n, "%02X", state->rows[r]);
        }
        if (n + 1 < size) {
            buf[n++] = '\n';
            buf[n]   = '\0';
        }
    }
    return n < size ? n : size - 1;
}

#ifndef SMTD_UNIT_TEST

#    include QMK_KEYBOARD_H
#    include "print.h"

// From modules/svalboard/core/client_wrapper.h, declared here rather than
// included so this file does not depend on the module's include path.
bool client_wrapper_valid_id(uint32_t id);
bool __real_client_wrapper_receive(uint8_t *data, uint8_t length);

/*
 * Every wrapped host packet ([DD][client:4][protocol][...]) reaches
 * client_wrapper_receive() from the module's via_command_kb(). A wrapped
 * VIA command (protocol FE) on ANNOUNCE_CHANNEL from a client ID the board
 * issued is answered here, in place: the wrapper header stays, the VIA
 * reply replaces the command after it -- the same shape the module's own
 * wrapped VIA replies have. Anything else, including an expired ID (which
 * gets the module's error reply), goes on untouched.
 */
bool __wrap_client_wrapper_receive(uint8_t *data, uint8_t length) {
    if (length > 6 && data[5] == 0xFE && data[7] == ANNOUNCE_CHANNEL) {
        uint32_t id = data[1] | (uint32_t)data[2] << 8 | (uint32_t)data[3] << 16 | (uint32_t)data[4] << 24;
        if (client_wrapper_valid_id(id) && announce_handle_command(&data[6], length - 6, timer_read32())) {
            host_raw_hid_send(data, length);
            return true;
        }
    }
    return __real_client_wrapper_receive(data, length);
}

void announce_task(void) {
    if (!subscribed) {
        return;
    }

    announce_state_t state = {
        .active         = layer_state,
        .default_layers = default_layer_state,
        .mods           = get_mods() | get_oneshot_mods(),
        .status         = host_keyboard_led_state().caps_lock ? 0x02 : 0x00,
    };
#    ifdef CAPS_WORD_ENABLE
    if (is_caps_word_on()) {
        state.status |= 0x01;
    }
#    endif
    for (uint8_t r = 0; r < ANNOUNCE_ROWS; r++) {
        state.rows[r] = (uint8_t)matrix_get_row(r);
    }

    // Two lines at most: "svw L" is 29 bytes, "svw K" 7 + 2 per row.
    char buf[40 + 8 + 2 * ANNOUNCE_ROWS];
    if (announce_poll(&state, timer_read32(), buf, sizeof(buf)) > 0) {
        uprintf("%s", buf);
    }
}

#endif  // SMTD_UNIT_TEST
