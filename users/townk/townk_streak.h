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

#ifndef QMK_USERSPACE_TOWNK_STREAK_H
#define QMK_USERSPACE_TOWNK_STREAK_H

/**
 * @file townk_streak.h
 * @brief Typing streak: a space pressed mid-word is a space, not _NUM.
 *
 * CKC_SPC is Space on tap and the numbers layer on hold. Typing fast, the
 * thumb is often still on space when the next word's first letter goes
 * down AND up -- down SPC, down A, up A, up SPC. sm_td resolves that
 * nested press as a hold the instant A is released, with no timer
 * involved, so none of the SMTD_* terms can tell it from a deliberate
 * "hold space, tap 3". The letter came out as a digit.
 *
 * What does tell them apart is what came right BEFORE the space. In a word
 * in progress, the previous press is a typing key (letter, digit,
 * punctuation) a moment ago; a deliberate number starts after a pause, or
 * after something that is not typing. This is QMK's "flow tap" idea, which
 * sm_td does not have.
 *
 * So every key press is noted here, before sm_td sees it, and each CKC_SPC
 * press is classified once, when it goes down: "in flow" when the press
 * before it was a typing key less than TOWNK_SPACE_STREAK_TERM_MS ago.
 * CKC_SPC's hold (townk_smtd.c) asks streak_space_in_flow(), and in flow
 * types the space instead of raising _NUM -- whichever path sm_td took to
 * the hold (the nested press, an overlapping roll, or the tap-term
 * timeout).
 *
 * A space press is itself not a typing key: in "page 3" (letter, space
 * tap, space hold + digit) the second space follows a space, so it holds
 * normally even inside the term.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief How recent the previous typing key must be for a space to count as
 *        mid-word, in milliseconds.
 *
 * The trade-off: raise it if spaces still turn into numbers while typing;
 * lower it if a number typed right after a word (hold space, digit) comes
 * out as a space and a letter. 150 is QMK's own suggested flow-tap term.
 * Overridable from the keymap's config.h.
 */
#ifndef TOWNK_SPACE_STREAK_TERM_MS
#    define TOWNK_SPACE_STREAK_TERM_MS 150
#endif

/**
 * @brief Note one key event. Call from process_record_user() for every
 *        record, BEFORE sm_td sees it; releases are ignored.
 *
 * @param keycode The keycode QMK resolved for the event.
 * @param pressed Whether the event is a press.
 */
void streak_note(uint16_t keycode, bool pressed);

/**
 * @brief Was the latest CKC_SPC press made mid-word?
 *
 * Decided when the space went down, so the keys pressed while it is held
 * (the very ones that make sm_td resolve it) cannot change the answer.
 */
bool streak_space_in_flow(void);

/**
 * @brief Forget everything noted. Test seam.
 */
void streak_reset(void);

#endif // QMK_USERSPACE_TOWNK_STREAK_H
