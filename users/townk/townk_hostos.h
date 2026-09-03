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

#ifndef QMK_USERSPACE_TOWNK_HOSTOS_H
#define QMK_USERSPACE_TOWNK_HOSTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @file townk_hostos.h
 * @brief Pick the pointing profile (mac vs. PC scroll divisor) from the
 *        host's USB enumeration, robustly.
 *
 * Svalboard scales the scroll trackball by 120 on macOS, because macOS
 * ignores the hi-res scroll multiplier the firmware announces. It gets the
 * "is this a mac" answer from QMK's OS detection, which fingerprints the
 * host by the wLength of every string-descriptor read it ever makes. On
 * this Mac (macOS 26) the enumeration reads are
 *
 *     02 10  02 14  02 1C  |  04
 *
 * three paired length probes before SET_CONFIGURATION, then the 4-byte
 * LANGID read -- and no 0xFF. QMK's macOS rule needs a trailing 0xFF, so
 * that sequence falls through to the branch it labels "PS5" and reports
 * Linux; if some app happens to read a string with 0xFF before the 250 ms
 * debounce, the macOS rule matches instead. The verdict was a race, which
 * is why the scroll divisor came and went. And OS_DETECTION_KEYBOARD_RESET
 * (set by the Svalboard config) re-runs that race on every USB bus reset,
 * because it soft-reboots the keyboard. Observed on-device as SV_SOUT
 * reporting "qmk os: linux" beside these very reads.
 *
 * This module replaces the OS verdict with its own, built on three rules:
 *
 * 1. Only reads made BEFORE the host configures the device count. A real
 *    enumeration (any OS) reads its strings before SET_CONFIGURATION;
 *    later reads come from drivers and apps and are ignored.
 * 2. The chosen profile survives the keyboard's own soft resets (it lives
 *    in RAM the runtime never initialises), so a reboot on the same host
 *    starts from the right answer instead of the non-mac default.
 * 3. On such a warm boot, only a POSITIVE fingerprint (macOS's 0x02 probes,
 *    Windows's 0x04-and-0xFF reads) may change the remembered profile. An
 *    all-0xFF pattern -- Linux's signature, but also what a host that
 *    re-attaches a device it already knows can leave behind -- is not
 *    enough. A cold boot (power actually cycled: unplug, KVM switch)
 *    trusts every fingerprint, so a genuine Linux or Windows host is
 *    detected the moment it is plugged in.
 *
 * With no evidence at all, a cold boot starts in the mac profile: this
 * keyboard lives on Macs, and 120x too slow on a PC for the second it
 * takes to enumerate beats 120x too fast on a Mac indefinitely.
 */

/** The two pointing profiles Svalboard knows, plus "not decided yet". */
typedef enum {
    HOSTOS_UNKNOWN = 0,
    HOSTOS_MAC     = 1, /* scroll divisor 120, is_mac */
    HOSTOS_PC      = 2, /* scroll divisor 1 (Windows, Linux, everything else) */
} hostos_profile_t;

/** What the enumeration-time reads say, before policy is applied. */
typedef enum {
    HOSTOS_VERDICT_NONE = 0,
    HOSTOS_VERDICT_MAC,     /* macOS, iOS, iPadOS: paired 0x02 length probes */
    HOSTOS_VERDICT_WINDOWS, /* 0xFF reads plus the 0x04 LANGID read */
    HOSTOS_VERDICT_LINUX,   /* 0xFF reads, no 0x02 probes (also Quest, Switch) */
} hostos_verdict_t;

/**
 * @brief Profile a cold boot starts in, before any evidence arrives.
 *
 * Overridable from the keymap's config.h. HOSTOS_PC restores Svalboard's
 * stock behaviour.
 */
#ifndef TOWNK_HOSTOS_COLD_DEFAULT
#    define TOWNK_HOSTOS_COLD_DEFAULT HOSTOS_MAC
#endif

/**
 * @brief Apply a profile to the keyboard. Provided by the keymap.
 *
 * Called at boot (with the remembered or default profile) and whenever a
 * verdict changes the active profile. Must be idempotent.
 */
void hostos_profile_changed(hostos_profile_t profile);

/**
 * @brief Seed the profile at boot. Call from keyboard_post_init_user().
 *
 * Reads the warm memory left by a previous run on this power cycle; when
 * there is none (power was cycled), starts from TOWNK_HOSTOS_COLD_DEFAULT.
 */
void hostos_boot(void);

/**
 * @brief Record one string-descriptor read. Interrupt context.
 *
 * @param wlength    The read's wLength, as QMK's OS detection sees it.
 * @param configured Whether the host had already configured the device
 *                   when the read arrived (usb_connected_state()).
 */
void hostos_record_wlength(uint16_t wlength, bool configured);

/**
 * @brief Track the USB configuration state. Main-loop context.
 *
 * Call with every notify_usb_device_state_change_user() event; the module
 * finds the edges itself. Entering the configured state evaluates the
 * enumeration-time reads and applies the resulting profile.
 */
void hostos_usb_configured(bool configured);

/** @brief The profile currently applied to the keyboard. */
hostos_profile_t hostos_active_profile(void);

/**
 * @brief Classify enumeration-time read counts. Pure.
 *
 * QMK's fingerprint with the 0x02 length probes promoted to the deciding
 * signal: no trailing-0xFF requirement for macOS, no "PS5" rule turning
 * "3x 0x02 + 1x 0x04" into Linux.
 */
hostos_verdict_t hostos_classify(uint8_t count, uint8_t cnt_02, uint8_t cnt_04, uint8_t cnt_ff);

/**
 * @brief One line of diagnostics for SV_SOUT: boot kind, memory, active
 *        profile, this boot's verdict and counts, the previous boot's
 *        verdict, and the raw reads split at the configuration edge.
 * @return Characters written, excluding the terminator.
 */
int hostos_format_status(char *buf, size_t size);

/**
 * @brief Forget everything. Test seam.
 *
 * @param keep_memory true simulates a soft reset (the warm memory stays),
 *                    false a power cycle (the warm memory is invalidated).
 */
void hostos_forget(bool keep_memory);

#endif // QMK_USERSPACE_TOWNK_HOSTOS_H
