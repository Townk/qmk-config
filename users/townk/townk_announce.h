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

#ifndef QMK_USERSPACE_TOWNK_ANNOUNCE_H
#define QMK_USERSPACE_TOWNK_ANNOUNCE_H

/**
 * @file townk_announce.h
 * @brief Layer and switch changes, announced to a host that asked for them.
 *
 * The stock firmware only answers questions, so a host showing the current
 * layer and the held keys (tools/svalwatch) would have to keep asking. On
 * macOS each exchange costs ~240 us of CPU, mostly in the kernel's HID
 * stack, which makes a 60/s poll cost more than an always-on helper should.
 *
 * Instead, the host subscribes, and while the subscription lasts the board
 * prints one line to QMK's debug console for each change:
 *
 *     svw L <active:%08X> <default:%08X> <mods:%02X> <status:%02X>
 *     svw K <row 0:%02X> ... <row N-1:%02X>
 *
 * (no spaces between the K rows; status bit 0 = Caps Word, bit 1 = the
 * host's Caps Lock). The console is a separate HID interface from the Raw
 * HID one Keybard uses, so Keybard never sees these lines.
 *
 * The subscription is a lease: the host renews it every few seconds, and
 * each renewal re-announces the full state, which heals anything the host
 * missed. When the host stops renewing (it quit, crashed, the Mac slept)
 * the board goes quiet on its own. Without a subscriber nothing here runs
 * beyond one flag test per main-loop pass.
 *
 * The subscribe command is a VIA custom-value GET inside the module's
 * client wrapper (protocol FE), so firmware without this module hands it
 * to VIA, which hands it to svalboard.c, which ignores the channel and has
 * no value at ANNOUNCE_VALUE_SUBSCRIBE: a harmless no-op, and the reply
 * lacks the signature, so the host knows to poll instead:
 *
 *     request: DD <client> FE 08 57 F7 <lease_ms:u16 LE>
 *     reply:   DD <client> FE 08 57 F7 's' 'v' 'w' <ANNOUNCE_VERSION> <granted_ms:u16 LE>
 *
 * It reaches this module through the linker: the userspace rules.mk wraps
 * client_wrapper_receive (the module's wrapper entry point), so the call
 * from the module's dispatcher lands here first and everything else goes
 * on to the module untouched. The VIA-level handlers can't be wrapped that
 * way: quantum/via.c defines its own weak copies of them, and the linker
 * only reroutes calls into another file.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ANNOUNCE_CHANNEL 0x57          // 'W'; Svalboard uses 0x49, 0x53-0x55
#define ANNOUNCE_VALUE_SUBSCRIBE 0xF7  // no Svalboard value id that high
#define ANNOUNCE_VERSION 1
#define ANNOUNCE_LEASE_MIN_MS 500
#define ANNOUNCE_LEASE_MAX_MS 30000

// Both halves' rows: the master's matrix holds the other half's too.
#ifndef ANNOUNCE_ROWS
#    define ANNOUNCE_ROWS MATRIX_ROWS
#endif

typedef struct {
    uint32_t active;
    uint32_t default_layers;
    uint8_t  mods;
    uint8_t  status;
    uint8_t  rows[ANNOUNCE_ROWS];
} announce_state_t;

/**
 * @brief Handle a VIA custom-value command if it is a subscribe.
 *
 * @param data   VIA command buffer: [command][channel][value id][data...];
 *               rewritten in place with the reply.
 * @param length Usable bytes in @p data.
 * @param now    timer_read32() now.
 * @return true when the command was ours (the caller must not pass it on).
 */
bool announce_handle_command(uint8_t *data, uint8_t length, uint32_t now);

/** @brief Whether a lease is (as far as the last check knows) running. */
bool announce_subscribed(void);

/**
 * @brief Compare @p state with the last announced one and format the lines
 *        to print, if any.
 *
 * Ends the subscription (and returns 0) once the lease has run out.
 *
 * @return Bytes written to @p buf, without the terminating NUL.
 */
size_t announce_poll(const announce_state_t *state, uint32_t now, char *buf, size_t size);

/** @brief Drop the subscription (tests; also a fresh boot's state). */
void announce_reset(void);

/**
 * @brief Gather the live state and print what changed. Master only, from
 *        housekeeping_task_user().
 */
void announce_task(void);

#endif  // QMK_USERSPACE_TOWNK_ANNOUNCE_H
