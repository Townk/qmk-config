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
 * @file townk_hostos.c
 * @brief Host pointing-profile detection. See townk_hostos.h.
 */

#include "townk_hostos.h"

#include <stdio.h>
#include <string.h>

#ifndef SMTD_UNIT_TEST
#    include "os_detection.h" /* process_wlength(), the wrapped symbol */
#    include "usb_device_state.h"
#    include "usb_util.h" /* usb_connected_state() */
#endif

/* -------------------------------------------------------------------------
 * Warm memory: survives the keyboard's own resets, not a power cycle.
 *
 * ChibiOS never touches the .ram0.* input sections (QMK's RP2040 double-tap
 * bootloader marker lives there for the same reason), so whatever a previous
 * run left here is still there after soft_reset_keyboard(). After a power
 * cycle it is SRAM noise, which the magic and its complement reject. The
 * magic doubles as a layout version: bump it if the struct changes.
 * ---------------------------------------------------------------------- */

#define HOSTOS_MAGIC 0x484F5301u /* "HOS" + layout 1 */

typedef struct {
    uint32_t magic;
    uint8_t  profile;        /* hostos_profile_t last applied */
    uint8_t  warm_boots;     /* boots on this power cycle, saturating */
    uint8_t  prev_verdict;   /* hostos_verdict_t of the run that wrote this */
    uint8_t  prev_pre_reads; /* its enumeration-time read count */
    uint8_t  prev_post_reads;
    uint8_t  reserved[3];
    uint32_t guard; /* ~magic */
} hostos_memory_t;

#ifdef SMTD_UNIT_TEST
static hostos_memory_t memory;
#else
static hostos_memory_t __attribute__((section(".ram0.townk_hostos"))) memory;
#endif

static bool memory_valid(void) {
    return memory.magic == HOSTOS_MAGIC && memory.guard == (uint32_t)~HOSTOS_MAGIC;
}

/* -------------------------------------------------------------------------
 * Capture: every string-descriptor read of the current enumeration.
 * ---------------------------------------------------------------------- */

#define HOSTOS_MAX_READS 24

typedef struct {
    uint16_t wlength;
    bool     configured; /* read arrived after SET_CONFIGURATION */
} hostos_read_t;

typedef struct {
    hostos_read_t reads[HOSTOS_MAX_READS];
    uint8_t       stored;   /* entries kept (capped at HOSTOS_MAX_READS) */
    uint8_t       total;    /* all reads, saturating */
    uint8_t       pre_total; /* enumeration-time reads */
    uint8_t       pre_02;
    uint8_t       pre_04;
    uint8_t       pre_ff;
    uint8_t       post_total;
    bool          saw_configured; /* the host configured this enumeration */
} hostos_capture_t;

static volatile hostos_capture_t capture;

/* Boot-time facts, fixed by hostos_boot(). */
static bool             booted       = false;
static bool             warm_boot    = false;
static hostos_profile_t active       = HOSTOS_UNKNOWN;
static hostos_verdict_t last_verdict = HOSTOS_VERDICT_NONE;
static bool             usb_configured_seen = false;

/* The previous run's summary, copied out of the warm memory at boot before
 * this run overwrites it with its own. */
static uint8_t prior_verdict    = HOSTOS_VERDICT_NONE;
static uint8_t prior_pre_reads  = 0;
static uint8_t prior_post_reads = 0;

static void capture_clear(void) {
    memset((void *)&capture, 0, sizeof(capture));
}

static uint8_t sat_inc(uint8_t v) {
    return v == UINT8_MAX ? v : v + 1;
}

/* -------------------------------------------------------------------------
 * Classification and policy.
 * ---------------------------------------------------------------------- */

hostos_verdict_t hostos_classify(uint8_t count, uint8_t cnt_02, uint8_t cnt_04, uint8_t cnt_ff) {
    if (cnt_02 >= 2) {
        /* Paired length probes -- a 2-byte read for the length, then the
         * full read -- are how macOS, iOS and iPadOS fetch every string.
         * No Windows or Linux enumeration ever reads with wLength 2, so
         * this outranks everything else. QMK instead requires a trailing
         * 0xFF read and files "3x 0x02 + 1x 0x04" under PS5 (Linux); on
         * this Mac the LANGID read is that 0x04 and the 0xFF may never
         * come, which is the misdetection this module exists for. */
        return HOSTOS_VERDICT_MAC;
    }
    if (cnt_ff >= 2 && cnt_04 >= 1) {
        return HOSTOS_VERDICT_WINDOWS;
    }
    if (count >= 3 && cnt_ff >= 1 && cnt_04 == 0) {
        /* Linux (all 0xFF), Quest 2, Nintendo Switch. */
        return HOSTOS_VERDICT_LINUX;
    }
    return HOSTOS_VERDICT_NONE;
}

static hostos_profile_t profile_of(hostos_verdict_t verdict) {
    switch (verdict) {
        case HOSTOS_VERDICT_MAC:
            return HOSTOS_MAC;
        case HOSTOS_VERDICT_WINDOWS:
        case HOSTOS_VERDICT_LINUX:
            return HOSTOS_PC;
        default:
            return HOSTOS_UNKNOWN;
    }
}

static void remember(void) {
    memory.magic          = HOSTOS_MAGIC;
    memory.profile        = (uint8_t)active;
    memory.prev_verdict   = (uint8_t)last_verdict;
    memory.prev_pre_reads = capture.pre_total;
    memory.prev_post_reads = capture.post_total;
    memory.guard          = (uint32_t)~HOSTOS_MAGIC;
}

static void apply(hostos_profile_t profile) {
    if (profile == HOSTOS_UNKNOWN) {
        return;
    }
    if (profile != active) {
        active = profile;
        hostos_profile_changed(profile);
    }
    remember();
}

/**
 * The enumeration is complete: decide.
 *
 * A warm boot means the keyboard reset itself while the host kept it
 * powered -- same host, different enumeration. macOS's cached re-attach of
 * a known device skips the 0x02 probes and leaves an all-0xFF pattern that
 * is indistinguishable from Linux, so on a warm boot that pattern (and only
 * that pattern) may not move a remembered profile. Everything else is
 * evidence a cached re-attach cannot produce.
 */
static void evaluate(void) {
    hostos_verdict_t verdict = hostos_classify(capture.pre_total, capture.pre_02, capture.pre_04, capture.pre_ff);
    last_verdict             = verdict;

    if (verdict == HOSTOS_VERDICT_NONE) {
        remember();
        return;
    }
    if (verdict == HOSTOS_VERDICT_LINUX && warm_boot && memory.profile != HOSTOS_UNKNOWN) {
        remember();
        return;
    }
    apply(profile_of(verdict));
}

/* -------------------------------------------------------------------------
 * Entry points.
 * ---------------------------------------------------------------------- */

void hostos_boot(void) {
    warm_boot = memory_valid();
    if (warm_boot) {
        memory.warm_boots = sat_inc(memory.warm_boots);
        prior_verdict     = memory.prev_verdict;
        prior_pre_reads   = memory.prev_pre_reads;
        prior_post_reads  = memory.prev_post_reads;
    } else {
        memset(&memory, 0, sizeof(memory));
    }
    booted = true;

    hostos_profile_t seed = warm_boot ? (hostos_profile_t)memory.profile : HOSTOS_UNKNOWN;
    if (seed == HOSTOS_UNKNOWN) {
        seed = TOWNK_HOSTOS_COLD_DEFAULT;
    }
    active = seed;
    hostos_profile_changed(seed);
    remember();

    /* The host normally configures the device while the split halves are
     * still deciding who is master, so the event may already be in. */
    if (usb_configured_seen) {
        evaluate();
    }
}

void hostos_record_wlength(uint16_t wlength, bool configured) {
    /* A read while unconfigured, after this enumeration had been
     * configured, is the host enumerating again (bus reset without a
     * keyboard reset): start over. Exact, because it runs on the read
     * itself rather than on the main loop's delayed state event. */
    if (!configured && capture.saw_configured) {
        capture_clear();
    }
    if (configured) {
        capture.saw_configured = true;
    }

    if (capture.stored < HOSTOS_MAX_READS) {
        capture.reads[capture.stored].wlength    = wlength;
        capture.reads[capture.stored].configured = configured;
        capture.stored++;
    }
    capture.total = sat_inc(capture.total);

    if (configured) {
        capture.post_total = sat_inc(capture.post_total);
    } else {
        capture.pre_total = sat_inc(capture.pre_total);
        if (wlength == 0x02) {
            capture.pre_02 = sat_inc(capture.pre_02);
        } else if (wlength == 0x04) {
            capture.pre_04 = sat_inc(capture.pre_04);
        } else if (wlength == 0xFF) {
            capture.pre_ff = sat_inc(capture.pre_ff);
        }
    }

    /* Keep the warm memory's summary of this run current, so the next boot
     * can report it. Not before boot: the memory must stay untouched until
     * hostos_boot() has judged whether it is genuine. */
    if (booted) {
        memory.prev_pre_reads  = capture.pre_total;
        memory.prev_post_reads = capture.post_total;
    }
}

void hostos_usb_configured(bool configured) {
    bool edge           = configured && !usb_configured_seen;
    usb_configured_seen = configured;
    if (!edge) {
        return;
    }
    capture.saw_configured = true;
    if (booted) {
        evaluate();
    }
}

hostos_profile_t hostos_active_profile(void) {
    return active;
}

static const char *profile_name(uint8_t profile) {
    switch (profile) {
        case HOSTOS_MAC:
            return "mac";
        case HOSTOS_PC:
            return "pc";
        default:
            return "none";
    }
}

static const char *verdict_name(uint8_t verdict) {
    switch (verdict) {
        case HOSTOS_VERDICT_MAC:
            return "mac";
        case HOSTOS_VERDICT_WINDOWS:
            return "windows";
        case HOSTOS_VERDICT_LINUX:
            return "linux";
        default:
            return "none";
    }
}

int hostos_format_status(char *buf, size_t size) {
    int n = snprintf(buf, size, "hostos: %s boot #%u, active %s, verdict %s (pre %u: 02x%u 04x%u ffx%u; post %u), prev %s (%u/%u), reads:", warm_boot ? "warm" : "cold", (unsigned)memory.warm_boots, profile_name((uint8_t)active), verdict_name((uint8_t)last_verdict), capture.pre_total, capture.pre_02, capture.pre_04, capture.pre_ff, capture.post_total, verdict_name(prior_verdict), prior_pre_reads, prior_post_reads);
    if (n < 0) {
        return n;
    }

    bool divided = false;
    for (uint8_t i = 0; i < capture.stored && (size_t)n < size; i++) {
        if (capture.reads[i].configured && !divided) {
            n += snprintf(buf + n, size - (size_t)n, " |");
            divided = true;
        }
        if ((size_t)n < size) {
            n += snprintf(buf + n, size - (size_t)n, " %02X", capture.reads[i].wlength);
        }
    }
    if ((size_t)n < size && capture.total > capture.stored) {
        n += snprintf(buf + n, size - (size_t)n, " +%u", capture.total - capture.stored);
    }
    if ((size_t)n >= size) {
        n = (int)size - 1;
    }
    return n;
}

void hostos_forget(bool keep_memory) {
    if (!keep_memory) {
        memset(&memory, 0, sizeof(memory));
    }
    capture_clear();
    booted              = false;
    warm_boot           = false;
    active              = HOSTOS_UNKNOWN;
    last_verdict        = HOSTOS_VERDICT_NONE;
    usb_configured_seen = false;
    prior_verdict       = HOSTOS_VERDICT_NONE;
    prior_pre_reads     = 0;
    prior_post_reads    = 0;
}

/* -------------------------------------------------------------------------
 * QMK hooks. The firmware links with -Wl,--wrap=process_wlength (see
 * rules.mk), so every string-descriptor read the USB stack hands to QMK's
 * OS detection passes through here first, still in interrupt context.
 * ---------------------------------------------------------------------- */
#ifndef SMTD_UNIT_TEST

void __real_process_wlength(const uint16_t w_length);

void __wrap_process_wlength(const uint16_t w_length) {
    hostos_record_wlength(w_length, usb_connected_state());
    __real_process_wlength(w_length);
}

void notify_usb_device_state_change_user(struct usb_device_state usb_device_state) {
    hostos_usb_configured(usb_device_state.configure_state == USB_DEVICE_STATE_CONFIGURED);
}

/**
 * QMK's own verdict never reaches Svalboard's handler: returning false
 * makes process_detected_host_os_kb() bail before touching the divisors.
 * The raw verdict stays readable through detected_host_os() for SV_SOUT.
 */
bool process_detected_host_os_user(os_variant_t os) {
    (void)os;
    return false;
}

#endif // SMTD_UNIT_TEST
