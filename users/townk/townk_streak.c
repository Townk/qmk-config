/**
 * @file townk_streak.c
 * @brief Typing streak for CKC_SPC. See townk_streak.h.
 */

#include "townk_streak.h"

#include "townk_keycodes.h"

#ifndef SMTD_UNIT_TEST
/* timer_read32() / timer_elapsed32(), and the KC_* and LSFT() keycodes.
 * The host fixture supplies all of these before including this file. */
#    include "quantum.h"
#endif

/** Whether the latest non-space press was a typing key, and when. */
static bool     prev_was_typing = false;
static uint32_t prev_time       = 0;

/** The verdict for the latest CKC_SPC press, fixed when it went down. */
static bool space_in_flow = false;

/**
 * @brief Does this keycode type text?
 *
 * Letters, digits and the punctuation keys, plain or shifted (the base
 * layer carries S(KC_1) "!" and the parentheses as such). Space is
 * deliberately absent, and so are Enter, Esc, Backspace and Tab: none of
 * them sits inside a word. Modifiers, layer keys and everything else 16-bit
 * are not typing either.
 */
static bool is_typing_key(uint16_t keycode) {
    uint16_t basic = keycode & 0xFF;

    if (keycode != basic && keycode != LSFT(basic)) {
        return false;
    }
    return (basic >= KC_A && basic <= KC_0) || (basic >= KC_MINS && basic <= KC_SLSH);
}

void streak_note(uint16_t keycode, bool pressed) {
    if (!pressed) {
        return;
    }

    if (keycode == CKC_SPC) {
        space_in_flow   = prev_was_typing && timer_elapsed32(prev_time) < TOWNK_SPACE_STREAK_TERM_MS;
        prev_was_typing = false;
        return;
    }

    prev_was_typing = is_typing_key(keycode);
    prev_time       = timer_read32();
}

bool streak_space_in_flow(void) {
    return space_in_flow;
}

void streak_reset(void) {
    prev_was_typing = false;
    prev_time       = 0;
    space_in_flow   = false;
}
