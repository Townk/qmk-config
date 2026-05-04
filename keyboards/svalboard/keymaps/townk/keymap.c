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

#include <stdbool.h>
#include <stdint.h>
#include "keycodes.h"
#include "modifiers.h"

#include QMK_KEYBOARD_H
#include "quantum_keycodes.h"
#include "townk_layers.h"
#include "townk_keycodes.h"
#include "townk_mouse.h"
#include "townk_overrides.h"

extern void mouse_mode(bool on);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * DISCLAIMER                                                                *
 * ----------                                                                *
 * This layout docs use several charcters only found on the NerdFonts fonts  *
 * (https://www.nerdfonts.com), so, to proper visualize this file, use one   *
 * of the NerdFonts fonts and TURN-OFF the auto-line-wrap on your editor.    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * To understand this keymap, you need to understand the terminology used on
 * Svalboard.
 *
 * The following schema is a cheat-sheet of the terms laid out on a keyboard
 * representation.
 *
 *       L4           L3           L2           L1      ┊      R1           R2           R3           R4
 *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
 *     │ N │        │ N │        │ N │        │ N │     ┊     │ N │        │ N │        │ N │        │ N │
 * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
 * │ W │ C │ E ││ W │ C │ E ││ W │ C │ E ││ W │ C │ E │ ┊ │ W │ C │ E ││ W │ C │ E ││ W │ C │ E ││ W │ C │ E │
 * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
 *     │ S │        │ S │        │ S │        │ S │     ┊     │ S │        │ S │        │ S │        │ S │
 *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
 *     │D-S│        │D-S│        │D-S│        │D-S│     ┊     │D-S│        │D-S│        │D-S│        │D-S│
 *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
 *                                                      ┊
 *                                   LT (Left Thumbs)   ┊   RT (Right Thumbs)
 *                                 ╭─────╭────╮──────╮  ┊  ╭──────╭────╮─────╮
 *                                 │ Pad ││DD││ Nail │  ┊  │ Nail ││DD││ Pad │
 *                                 ╰─────│╰──╯│──────╯  ┊  ╰──────│╰──╯│─────╯
 *                                  │ Up │    │───────╮ ┊ ╭───────│    │ Up │
 *                                  ╰────│Down│Knuckle│ ┊ │Knuckle│Down│────╯
 *                                       ╰────╯───────╯ ┊ ╰───────╰────╯
 *
 * Each half of a Svalboard has a "finger cluster" and a "thumb cluster".
 *
 * Finger clusters are labeled by a letter (`L` or `R`) indicating which half
 * of the keyboard they belong to, and a number (`1`, `2`, `3`, or `4`)
 * indicating which finger is used to activate keys in the cluster, numbered
 * from the inside to the outside when your palms are facing down.
 *
 * Keys on finger clusters are named after their physical position on the
 * keyboard:
 *
 * - `C` (_Center Key_): Activated with a downward press of the finger.
 * - `N` (_North Key_): Activated with an upward flick of the finger.
 * - `E` (_East Key_): Activated with a lateral movement to the right.
 * - `S` (_South Key_): Activated with a downward flick (like scratching) of
 *   the finger.
 * - `W` (_West Key_): Activated with a lateral movement to the left.
 * - `D-S` (_Double-South Key_): Positioned behind the South Key and activated
 *   the same way.
 *
 * The thumb cluster keys are named after the part of the thumb used to
 * activate them:
 *
 * - `Pad`: Activated with an inward motion of the thumb.
 * - `Nail`: Activated with an outward motion of the thumb.
 * - `Knuckle`: Activated with an outward motion of the thumb knuckle.
 * - `Up`: Activated with an upward motion of the thumb.
 * - `Down`: Activated with a downward motion of the thumb.
 * - `DD` (_Double-Down Key_): Activated by pressing the Down Key further after
 *   its initial activation.
 */

/**
 * KEYMAP OVERVIEW
 * ===============
 *
 * This keymap implements a highly optimized COLEMAK-DH layout with ergonomic
 * layer design inspired by the Miryoku layout philosophy. It emphasizes minimal
 * finger travel, comfortable chording, and logical key grouping across six
 * layers.
 *
 * LAYOUT PHILOSOPHY
 * -----------------
 *
 * **Miryoku-Inspired Layer Activation:**
 * Layers follow a single-hand activation pattern. When you hold a layer key on
 * one hand, all functional keys for that layer appear on the opposite hand.
 * This allows you to activate a layer and use it without complex finger
 * gymnastics or same-hand stretching.
 *
 * Example: Hold left thumb on CKC_BSPC → NAV layer keys appear on right hand.
 *
 * **Home Row Modifiers:**
 * The base layer places all four primary modifiers (⌃ ⌥ ⌘ ⇧) on the Double-
 * South keys of each finger cluster, accessible via a comfortable downward
 * flick. Modifiers mirror across both hands for convenience.
 *
 * **Dual-Function Thumb Keys:**
 * Thumb cluster keys serve double duty with tap/hold behavior:
 *   - CKC_BSPC: Backspace on tap, Navigation layer on hold
 *   - CKC_SPC:  Space on tap, Numbers layer on hold
 *   - CKC_TAB:  Tab on tap, Symbols layer on hold
 *   - CKC_BKTAB: Back-tab (⇧⇥) on tap, Function keys layer on hold
 *   - CKC_SMSFT: One-shot Shift on tap, Shift on hold, Caps Word on double-tap
 *
 * LAYER STRUCTURE
 * ---------------
 *
 * ⓿ BASE Layer (COLEMAK-DH):
 *   The primary typing layer featuring the COLEMAK-DH letter layout with
 *   frequently used symbols placed on the Center and side keys for easy access
 *   without leaving the base layer. Punctuation and brackets are positioned for
 *   programming efficiency.
 *
 * ➊ NAV Layer (Navigation):
 *   Activated by left thumb (CKC_BSPC hold). Contains all navigation keys on
 *   the right hand in Vim-style arrangement:
 *     - Arrow keys: H(←) J(↓) K(↑) L(→) following Vim conventions
 *     - Page navigation: Page Up/Down, Home/End
 *     - macOS window management: Mission Control, App Windows, Desktop
 *       switching
 *     - Word/line navigation with modifier combos (⌃←/→, ⌥←/→)
 *
 * ➋ NUM Layer (Numbers):
 *   Activated by right thumb (CKC_SPC hold). Numeric keypad layout on left hand
 *   with mathematical operators:
 *     - Numbers arranged from 1-9 (bottom-to-top, outside-to-inside)
 *     - 0 positioned for thumb access
 *     - Arithmetic operators (+, -, *, /) on North keys
 *     - Parentheses and equals sign for formula entry
 *
 * ➌ SYM Layer (Symbols):
 *   Activated by left thumb (CKC_TAB hold). Programming symbols on right hand,
 *   organized in logical pairs to facilitate chording:
 *     - Brackets: {}, [], ()
 *     - Comparison: <, >, =
 *     - Logic: &, |, !
 *     - Special: @, #, $, %, ^, *, /, \, _, :
 *   Symbols are arranged to minimize finger travel for common programming
 *   patterns (e.g., {} for blocks, [] for arrays, () for functions).
 *
 * ➍ FUN Layer (Function Keys):
 *   Activated by right thumb (CKC_BKTAB hold). All function keys F1-F20 on
 *   left hand:
 *     - F1-F4: South keys (bottom row)
 *     - F5-F9: Center and North keys (standard function keys)
 *     - F10-F20: Extended function keys for advanced workflows
 *   Arranged from bottom-to-top, outside-to-inside for systematic access.
 *
 * MBO Layer (Mouse Buttons):
 *   Auto-activated by trackball movement (if auto_mouse is enabled). Provides
 *   mouse button access with modifier keys:
 *     - MB_SFT, MB_ALT, MB_GUI, MB_CTL: Dual-function modifier/mouse buttons
 *     - Allows clicking with modifiers held (⌘-click, ⌃-click, etc.)
 *     - Sniper mode available for precision pointing
 *
 * KEY ORGANIZATION PRINCIPLES
 * ---------------------------
 *
 * - Numbers/Functions: Arranged bottom-to-top (1→9, F1→F9), outside-to-inside
 * - Symbols: Paired logically for chording (e.g., {} and [] adjacent)
 * - Navigation: Vim-style HJKL arrangement with contextual modifiers
 * - Modifiers: Symmetrically placed on both hands for flexibility
 * - Frequent keys: Placed on strong fingers (index, middle)
 *
 * SPECIAL FEATURES
 * ----------------
 *
 * - Dual trackball support with independent DPI settings
 * - RGB layer indicators (colors change per active layer)
 * - Auto mouse layer activation on trackball movement
 * - Caps Word support (type in CAPS without holding shift)
 * - Repeat key (QK_REP) for efficient character repetition
 * - Dynamic keymap support for runtime customization via Vial/Keybard
 *
 * REFERENCES
 * ----------
 *
 * - Miryoku Layout: https://github.com/manna-harbour/miryoku
 * - COLEMAK-DH: https://colemakmods.github.io/mod-dh/
 * - Svalboard: https://svalboard.com
 *
 * @see townk_keycodes.h for custom keycode definitions
 * @see townk_layers.h for layer enumeration and RGB configuration
 * @see townk_mouse.h for trackball and mouse layer behavior
 */
const uint16_t PROGMEM keymaps[DYNAMIC_KEYMAP_LAYER_COUNT][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * COLEMAK-DH Layer (⓿ BASE)
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │ Q │        │ W │        │ F │        │ P │     ┊     │ L │        │ U │        │ Y │        │ ; │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │ [ │ A │ ( ││ \ │ R │ V ││ ` │ S │ B ││ ' │ T │ G │ ┊ │ M │ N │ - ││ J │ E │ = ││ K │ I │ ! ││ ) │ O │ ] │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │ Z │        │ X │        │ C │        │ D │     ┊     │ H │        │ , │        │ . │        │ / │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │⌫|⌦|➊││⇧W││ ⇥|➌ │ ┊ │ ⇤|➍ ││⇧W││ ␣|➋ │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │ 󰑓 │    │─────╮ ┊ ╭─────│    │ ⎋ │
     *                                     ╰───│ ⇧ │  ➎  │ ┊ │  ➏  │ ⏎  │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_BASE] = LAYOUT(
        /*     Center      North      East      South       West       Double-South */
        /*R1*/ KC_N,       KC_L,      KC_MINS,  KC_H,       KC_M,      KC_RIGHT_SHIFT,
        /*R2*/ KC_E,       KC_U,      KC_EQL,   KC_COMMA,   KC_J,      KC_RCMD,
        /*R3*/ KC_I,       KC_Y,      KC_EXLM,  KC_DOT,     KC_K,      KC_ROPT,
        /*R4*/ KC_O,       KC_SCLN,   KC_RBRC,  KC_SLASH,   KC_RPRN,   KC_RIGHT_CTRL,

        /*L1*/ KC_T,       KC_P,      KC_G,     KC_D,       KC_QUOT,   KC_LEFT_SHIFT,
        /*L2*/ KC_S,       KC_F,      KC_B,     KC_C,       KC_GRV,    KC_LCMD,
        /*L3*/ KC_R,       KC_W,      KC_V,     KC_X,       KC_BSLS,   KC_LOPT,
        /*L4*/ KC_A,       KC_Q,      KC_LPRN,  KC_Z,       KC_LBRC,   KC_LEFT_CTRL,

        /*     Down        Pad        Up        Nail        Knuckle    Double Down  */
        /*RT*/ KC_ENTER,   CKC_SPC,   KC_ESC,   CKC_BKTAB,  MO(_SYS),  QK_CAPS_WORD_TOGGLE,
        /*LT*/ CKC_SMSFT,  CKC_BSPC,  QK_REP,   CKC_TAB,    MO(_MED),  QK_CAPS_WORD_TOGGLE
        ),

    /*
     * QWERTY Layer (➊ BASE)
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │ Q │        │ W │        │ E │        │ R │     ┊     │ U │        │ I │        │ O │        │ P │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │ [ │ A │ ( ││ \ │ S │ B ││ ` │ D │ T ││ ' │ F │ G │ ┊ │ H │ J │ - ││ Y │ K │ = ││ N │ L │ ! ││ ) │ ; │ ] │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │ Z │        │ X │        │ C │        │ V │     ┊     │ M │        │ , │        │ . │        │ / │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │⌫|⌦|➊││⇧W││ ⇥|➌ │ ┊ │ ⇤|➍ ││⇧W││ ␣|➋ │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │ 󰑓 │    │─────╮ ┊ ╭─────│    │ ⎋ │
     *                                     ╰───│ ⇧ │  ➎  │ ┊ │  ➏  │ ⏎  │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_QWT] = LAYOUT(
        /*     Center      North      East      South       West       Double-South */
        /*R1*/ KC_J,       KC_L,      KC_MINS,  KC_M,       KC_H,      KC_RIGHT_SHIFT,
        /*R2*/ KC_K,       KC_I,      KC_EQL,   KC_COMMA,   KC_Y,      KC_RCMD,
        /*R3*/ KC_L,       KC_O,      KC_EXLM,  KC_DOT,     KC_N,      KC_ROPT,
        /*R4*/ KC_SCLN,    KC_P,      KC_RBRC,  KC_SLASH,   KC_RPRN,   KC_RIGHT_CTRL,

        /*L1*/ KC_F,       KC_R,      KC_G,     KC_V,       KC_QUOT,   KC_LEFT_SHIFT,
        /*L2*/ KC_D,       KC_E,      KC_T,     KC_C,       KC_GRV,    KC_LCMD,
        /*L3*/ KC_S,       KC_W,      KC_B,     KC_X,       KC_BSLS,   KC_LOPT,
        /*L4*/ KC_A,       KC_Q,      KC_LPRN,  KC_Z,       KC_LBRC,   KC_LEFT_CTRL,

        /*     Down        Pad        Up        Nail        Knuckle    Double Down  */
        /*RT*/ KC_ENTER,   CKC_SPC,   KC_ESC,   CKC_BKTAB,  MO(_SYS),  QK_CAPS_WORD_TOGGLE,
        /*LT*/ CKC_SMSFT,  CKC_BSPC,  QK_REP,   CKC_TAB,    MO(_MED),  QK_CAPS_WORD_TOGGLE
        ),

    /*
     * Game Layer (➋ GAM1)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │ ⌥ │        │ Q │        │ C │        │ E │     ┊     │   │        │   │        │   │        │   │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │ H │ ⇧ │ ` ││ Z │ A │ X ││ U │ W │ F ││ T │ D │ R │ ┊ │   │   │   ││   │   │   ││   │   │   ││   │   │   │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │ ⌃ │        │ M │        │ S │        │ V │     ┊     │   │        │   │        │   │        │   │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ N │        │ G │        │ Y │        │ B │     ┊     │   │        │   │        │   │        │   │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │  ␣  ││➊ ││  ⇥  │ ┊ │ 󰶼  ││➊ ││ 1  │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │ ⎋ │    │─────╮ ┊ ╭─────│    │ ⎋ │
     *                                     ╰───│ ➌  │  ⌘  │ ┊ │ 󰶹  │ 3 │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_GAM1] = LAYOUT(
        /*     Center      North     East      South     West      Double-South */
        /*R1*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R2*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R3*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R4*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,

        /*L1*/ KC_D,       KC_E,     KC_R,     KC_V,     KC_T,     KC_B,
        /*L2*/ KC_W,       KC_C,     KC_F,     KC_S,     KC_U,     KC_Y,
        /*L3*/ KC_A,       KC_Q,     KC_X,     KC_M,     KC_Z,     KC_G,
        /*L4*/ KC_LSFT,    KC_LOPT,  KC_GRV,   KC_LCTL,  KC_H,     KC_N,

        /*     Down        Pad       Up        Nail      Knuckle   Double Down  */
        /*RT*/ KC_BTN2,    KC_BTN1,  KC_ESC,   KC_WH_U,  KC_WH_D,  TO(_BASE),
        /*LT*/ MO(_GAM2),  KC_SPC,   KC_ESC,   KC_TAB,   KC_LCMD,  TO(_BASE)
        ),

    /*
     * Game Layer (➌ GAM2)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │ 󱊱 │        │ 7 │        │ 8 │        │ 9 │     ┊     │   │        │   │        │   │        │   │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │ 󱊲 │ 󱊰 │ 󱊳 ││   │ 4 │   ││   │ 5 │   ││   │ 6 │ 0 │ ┊ │   │   │   ││   │   │   ││   │   │   ││   │   │   │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │ 󱊯 │        │ 1 │        │ 2 │        │ 3 │     ┊     │   │        │   │        │   │        │   │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ 󱊫 │        │ 󱊬 │        │ 󱊭 │        │ 󱊮 │     ┊     │   │        │   │        │   │        │   │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │  ⛛  ││⛛ ││  ⛛  │ ┊ │  ⛛  ││⛛ ││  ⛛  │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │ ⛛ │    │─────╮ ┊ ╭─────│    │ ⛛ │
     *                                     ╰───│ 🮖🮖 │  ⛛  │ ┊ │  ⛛  │ ⛛  │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_GAM2] = LAYOUT(
        /*     Center      North     East      South     West      Double-South */
        /*R1*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R2*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R3*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*R4*/ XXXXXXX,    XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,

        /*L1*/ KC_6,       KC_9,     KC_0,     KC_3,     XXXXXXX,  KC_F4,
        /*L2*/ KC_5,       KC_8,     XXXXXXX,  KC_2,     XXXXXXX,  KC_F3,
        /*L3*/ KC_4,       KC_7,     XXXXXXX,  KC_1,     XXXXXXX,  KC_F2,
        /*L4*/ KC_F6,      KC_F7,    KC_F9,    KC_F5,    KC_F8,    KC_F1,

        /*     Down        Pad       Up        Nail      Knuckle   Double Down  */
        /*RT*/ _______,    _______,  _______,  _______,  _______,  _______,
        /*LT*/ XXXXXXX,    _______,  _______,  _______,  _______,  _______
        ),

    /*
     * Navigation Layer (➍ NAV)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │   │        │   │        │   │        │   │     ┊     │⌃⌘←│        │⌃⌘↓│        │⌃⌘↑│        │⌃⌘→│
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │   │   │   ││   │   │   ││   │   │   ││   │   │   │ ┊ │ ↖ │ ← │ ↘ ││⌥ ←│ ↓ │⌥ →││⌃ ←│ ↑ │⌃ →││ ⇞ │ → │ ⇟ │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │   │        │   │        │   │        │   │     ┊     │⌥⌘←│        │⌥⌘↓│        │⌥⌘↑│        │⌥⌘→│
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │ 🮖🮖🮖 ││  ││     │ ┊ │  ⇤  ││  ││  ⇥  │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │   │    │─────╮ ┊ ╭─────│    │   │
     *                                     ╰───│    │     │ ┊ │     │    │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_NAV] = LAYOUT(
        /*     Center     North        East         South      West         Double-South */
        /*R1*/ KC_LEFT,   MKC_DKTP,    KC_END,      KC_LPAD,   KC_HOME,     KC_RIGHT_SHIFT,
        /*R2*/ KC_DOWN,   MKC_APPWIN,  A(KC_RGHT),  MKC_SHDKT, A(KC_LEFT),  KC_RCMD,
        /*R3*/ KC_UP,     KC_MCTL,     G(KC_RGHT),  MKC_SHNOT, G(KC_LEFT),  KC_ROPT,
        /*R4*/ KC_RIGHT,  MKC_DKTN,    KC_PGDN,     MKC_SPFND, KC_PGUP,     KC_RIGHT_CTRL,

        /*L1*/ XXXXXXX,   XXXXXXX,     XXXXXXX,     XXXXXXX,   XXXXXXX,     KC_LEFT_SHIFT,
        /*L2*/ XXXXXXX,   XXXXXXX,     XXXXXXX,     XXXXXXX,   XXXXXXX,     KC_LCMD,
        /*L3*/ XXXXXXX,   XXXXXXX,     XXXXXXX,     XXXXXXX,   XXXXXXX,     KC_LOPT,
        /*L4*/ XXXXXXX,   XXXXXXX,     XXXXXXX,     XXXXXXX,   XXXXXXX,     KC_LEFT_CTRL,

        /*     Down       Pad          Up           Nail       Knuckle      Double Down  */
        /*RT*/ XXXXXXX,   KC_TAB,      XXXXXXX,     MKC_BKTAB, XXXXXXX,     XXXXXXX,
        /*LT*/ XXXXXXX,   XXXXXXX,     XXXXXXX,     XXXXXXX,   XXXXXXX,     XXXXXXX
        ),

    /*
     * Numbers Layer (➎ NUM)
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │ / │        │ * │        │ - │        │ + │     ┊     │   │        │   │        │   │        │   │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │ 0 │ 5 │ , ││ ^ │ 6 │ = ││ ( │ 7 │ ) ││ . │ 8 │ 9 │ ┊ │   │   │   ││   │   │   ││   │   │   ││   │   │   │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │ 1 │        │ 2 │        │ 3 │        │ 4 │     ┊     │   │        │   │        │   │        │   │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │  ⌫  ││  ││  ␣  │ ┊ │     ││  ││ 🮖🮖🮖 │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │ 󰑓 │    │─────╮ ┊ ╭─────│    │   │
     *                                     ╰───│ ⌅  │  ⇥  │ ┊ │     │    │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_NUM] = LAYOUT(
        /*     Center    North     East      South     West      Double-South */
        /*R1*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RIGHT_SHIFT,
        /*R2*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RCMD,
        /*R3*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_ROPT,
        /*R4*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RIGHT_CTRL,

        /*L1*/ KC_KP_8,  KC_PPLS,  KC_KP_9,  KC_KP_4,  KC_PDOT,  KC_LEFT_SHIFT,
        /*L2*/ KC_KP_7,  KC_PMNS,  KC_RPRN,  KC_KP_3,  KC_LPRN,  KC_LCMD,
        /*L3*/ KC_KP_6,  KC_PAST,  KC_PEQL,  KC_KP_2,  KC_CIRC,  KC_LOPT,
        /*L4*/ KC_KP_5,  KC_PSLS,  KC_COMM,  KC_KP_1,  KC_KP_0,  KC_LEFT_CTRL,

        /*     Down      Pad       Up        Nail      Knuckle   Double Down  */
        /*RT*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*LT*/ KC_PENT,  KC_BSPC,  _______,  KC_SPC,   KC_TAB,   XXXXXXX
        ),

    /*
     * Symbols Layer (➏ SYM)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │   │        │   │        │   │        │   │     ┊     │ < │        │ = │        │ > │        │ _ │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │   │   │   ││   │   │   ││   │   │   ││   │   │   │ ┊ │ ^ │ { │ $ ││ & │ : │ | ││ ! │ } │ % ││ / │ @ │ \ │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │   │        │   │        │   │        │   │     ┊     │ [ │        │ * │        │ ] │        │ # │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │     ││  ││ 🮖🮖🮖 │ ┊ │  ⇤  ││  ││  ␣  │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │   │    │─────╮ ┊ ╭─────│    │ ⎋ │
     *                                     ╰───│    │     │ ┊ │     │  ⏎ │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_SYM] = LAYOUT(
        /*     Center    North      East      South      West      Double-South */
        /*R1*/ KC_LCBR,  KC_LT,     KC_DLR,   KC_LBRC,   KC_CIRC,  KC_RIGHT_SHIFT,
        /*R2*/ KC_COLN,  KC_EQUAL,  KC_PIPE,  KC_ASTR,   KC_AMPR,  KC_RCMD,
        /*R3*/ KC_RCBR,  KC_GT,     KC_PERC,  KC_RBRC,   KC_EXLM,  KC_ROPT,
        /*R4*/ KC_AT,    KC_UNDS,   KC_BSLS,  KC_HASH,   KC_SLSH,  KC_RIGHT_CTRL,

        /*L1*/ XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,   XXXXXXX,  KC_LEFT_SHIFT,
        /*L2*/ XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,   XXXXXXX,  KC_LCMD,
        /*L3*/ XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,   XXXXXXX,  KC_LOPT,
        /*L4*/ XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,   XXXXXXX,  KC_LEFT_CTRL,

        /*     Down      Pad        Up        Nail       Knuckle   Double Down  */
        /*RT*/ _______,  KC_SPC,    QK_REP,   KC_BSPC,   KC_TAB,   XXXXXXX,
        /*LT*/ XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX,   XXXXXXX,  XXXXXXX
        ),

    /*
     * Function Keys Layer (➐ FUN)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │F11│        │F12│        │F13│        │F14│     ┊     │   │        │   │        │   │        │   │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │F10│F5 │F15││F16│F6 │F17││F18│F7 │F19││F20│F8 │F9 │ ┊ │   │   │   ││   │   │   ││   │   │   ││   │   │   │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │F1 │        │F2 │        │F3 │        │F4 │     ┊     │   │        │   │        │   │        │   │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │     ││  ││     │ ┊ │ 🮖🮖🮖 ││  ││     │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │   │    │─────╮ ┊ ╭─────│    │   │
     *                                     ╰───│    │     │ ┊ │     │    │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_FUN] = LAYOUT(
        /*     Center    North     East      South     West      Double-South */
        /*R1*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RIGHT_SHIFT,
        /*R2*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RCMD,
        /*R3*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_ROPT,
        /*R4*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RIGHT_CTRL,

        /*L1*/ KC_F8,    KC_F14,   KC_F9,    KC_F4,    KC_F20,   KC_LEFT_SHIFT,
        /*L2*/ KC_F7,    KC_F13,   KC_F19,   KC_F3,    KC_F18,   KC_LCMD,
        /*L3*/ KC_F6,    KC_F12,   KC_F17,   KC_F2,    KC_F16,   KC_LOPT,
        /*L4*/ KC_F5,    KC_F11,   KC_F15,   KC_F1,    KC_F10,   KC_LEFT_CTRL,

        /*     Down      Pad       Up        Nail      Knuckle   Double Down  */
        /*RT*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*LT*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX
        ),

    /*
     * Muiti-Media Layer (➑ MED)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │   │        │   │        │   │        │   │     ┊     │ 󰝝 │        │ 󰒭 │        │   │        │ ⏏ │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │   │   │   ││   │   │   ││   │   │   ││   │   │   │ ┊ │   │ 󰖁 │   ││ 󰑟 │ 󰐎 │ 󰈑 ││   │   │   ││   │   │   │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │   │        │   │        │   │        │   │     ┊     │ 󰝞 │        │ 󰒮 │        │   │        │   │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │     ││  ││     │ ┊ │     ││  ││     │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │   │    │─────╮ ┊ ╭─────│    │   │
     *                                     ╰───│    │ 🮖🮖🮖 │ ┊ │     │    │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_MED] = LAYOUT(
        /*     Center    North     East      South     West      Double-South */
        /*R1*/ KC_MUTE,  KC_VOLU,  XXXXXXX,  KC_VOLD,  XXXXXXX,  KC_RIGHT_SHIFT,
        /*R2*/ KC_MPLY,  KC_MFFD,  KC_MNXT,  KC_MRWD,  KC_MPRV,  KC_RCMD,
        /*R3*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_ROPT,
        /*R4*/ XXXXXXX,  KC_EJCT,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RIGHT_CTRL,

        /*L1*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_LEFT_SHIFT,
        /*L2*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_LCMD,
        /*L3*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_LOPT,
        /*L4*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_LEFT_CTRL,

        /*     Down      Pad       Up        Nail      Knuckle   Double Down  */
        /*RT*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*LT*/ XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX
        ),

    /*
     * System Layer (⓮ SYS)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │   │        │L󱕑 │        │R󱕑 │        │+󰃠 │     ┊     │   │        │   │        │   │        │   │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │   │  │   ││   │   │   ││   │   │   ││   │   │   │ ┊ │   │   │   ││   │ ⓿ │   ││   │ ➊ │   ││   │ ➋ │   │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │ 󰒲 │        │L󱕐 │        │R󱕐 │        │-󰃞 │     ┊     │   │        │   │        │   │        │   │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │     ││  ││     │ ┊ │     ││  ││     │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │   │    │─────╮ ┊ ╭─────│    │   │
     *                                     ╰───│ 󱇗  │     │ ┊ │ 🮖🮖🮖 │    │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_SYS] = LAYOUT(
        /*     Center     North     East      South     West      Double-South */
        /*R1*/ XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RIGHT_SHIFT,
        /*R2*/ TO(_GAM1), XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RCMD,
        /*R3*/ TO(_QWT),  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_ROPT,
        /*R4*/ TO(_BASE), XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  KC_RIGHT_CTRL,

        /*L1*/ XXXXXXX,   KC_BRIU,  XXXXXXX,  KC_BRID,  XXXXXXX,  KC_LEFT_SHIFT,
        /*L2*/ XXXXXXX,   SV_RDPU,  XXXXXXX,  SV_RDPD,  XXXXXXX,  KC_LCMD,
        /*L3*/ XXXXXXX,   SV_LDPU,  XXXXXXX,  SV_LDPD,  XXXXXXX,  KC_LOPT,
        /*L4*/ KC_PWR,    XXXXXXX,  XXXXXXX,  KC_SLEP,  XXXXXXX,  KC_LEFT_CTRL,

        /*     Down       Pad       Up        Nail      Knuckle   Double Down  */
        /*RT*/ XXXXXXX,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,
        /*LT*/ SV_SOUT,   XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX,  XXXXXXX
        ),

    /*
     * Mouse Buttons Layer (⓯ MBO)
     *
     *      L4           L3           L2           L1       ┊      R1           R2           R3           R4
     *     ╭───╮        ╭───╮        ╭───╮        ╭───╮     ┊     ╭───╮        ╭───╮        ╭───╮        ╭───╮
     *     │ ⛛ │        │ ⛛ │        │ ⛛ │        │ ⛛ │     ┊     │ ⛛ │        │ ⛛ │        │ ⛛ │        │ ⛛ │
     * ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮ ┊ ╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮╭───┼───┼───╮
     * │ ⛛ │ ⛛ │ ⛛ ││ ⛛ │ ⛛ │ ⛛ ││ ⛛ │ ⛛ │ ⛛ ││ ⛛ │ ⛛ │ ⛛ │ ┊ │ ⛛ │ ⛛ │ ⛛ ││ ⛛ │ ⛛ │ ⛛ ││ ⛛ │ ⛛ │ ⛛ ││ ⛛ │ ⛛ │ ⛛ │
     * ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯ ┊ ╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯╰───┼───┼───╯
     *     │ ⛛ │        │ ⛛ │        │ ⛛ │        │ ⛛ │     ┊     │ ⛛ │        │ ⛛ │        │ ⛛ │        │ ⛛ │
     *     ├───┤        ├───┤        ├───┤        ├───┤     ┊     ├───┤        ├───┤        ├───┤        ├───┤
     *     │ ⌃ │        │ ⌥ │        │ ⌘ │        │ ⇧ │     ┊     │ ⇧ │        │ ⌘ │        │ ⌥ │        │ ⌃ │
     *     ╰───╯        ╰───╯        ╰───╯        ╰───╯     ┊     ╰───╯        ╰───╯        ╰───╯        ╰───╯
     *                                                      ┊
     *                                    LT (Left Thumbs)  ┊  RT (Right Thumbs)
     *                                   ╭─────╭────╮─────╮ ┊ ╭─────╭────╮─────╮
     *                                   │⌫|⌦|➊││3󰓾││ ⇥|➌ │ ┊ │ ⇤|➍ ││5󰓾││ ␣|➋ │
     *                                   ╰─────│╰──╯│─────╯ ┊ ╰─────│╰──╯│─────╯
     *                                     │ ⌥ │    │─────╮ ┊ ╭─────│    │ ⎋ │
     *                                     ╰───│ ⇧  │  ➎  │ ┊ │  ➏  │ ⏎  │───╯
     *                                         ╰────╯─────╯ ┊ ╰─────╰────╯
     */
    [_MBO] = LAYOUT(
        /*     Center     North    East      South       West      Double-South  */
        /*R1*/ _______,   _______, _______,  _______,    _______,  KC_RIGHT_SHIFT,
        /*R2*/ _______,   _______, _______,  _______,    _______,  KC_RCMD,
        /*R3*/ _______,   _______, _______,  _______,    _______,  KC_ROPT,
        /*R4*/ _______,   _______, _______,  _______,    _______,  KC_RIGHT_CTRL,

        /*L1*/ _______,  _______,  _______,  _______,    _______,  MB_SFT,
        /*L2*/ _______,  _______,  _______,  _______,    _______,  MB_GUI,
        /*L3*/ _______,  _______,  _______,  _______,    _______,  MB_ALT,
        /*L4*/ _______,  _______,  _______,  _______,    _______,  MB_CTL,

        /*     Down      Pad       Up        Nail        Knuckle   Double Down   */
        /*RT*/ _______,  CKC_SPC,  MB_ESC,   CKC_BKTAB,  _______,  SV_SNIPER_5,
        /*LT*/ KC_LSFT,  CKC_BSPC, ML_CMD,   CKC_TAB,    _______,  SV_SNIPER_3
        )
};

/**
 * @brief User-level keyboard initialization hook
 *
 * This function is called after QMK's keyboard initialization is complete. It
 * configures the default values for the Svalboard's persistent settings, and
 * initializes RGB lighting and dynamic keymap support.
 *
 * **Configuration:**
 * - Left trackball: Scroll mode enabled, 400 DPI.
 * - Right trackball: Scroll mode disabled, 1200 DPI.
 * - Auto mouse layer: Enabled (automatically activates mouse layer).
 * - Mouse hold timer: Disabled (mouse layer stays active indefinitely).
 *
 * @note These are initialization values only. The actual values used at
 *       runtime are loaded from EEPROM by the keyboard-level initialization in
 *       svalboard.c
 *
 * @see keyboard_post_init_kb() in svalboard.c for EEPROM loading.
 * @see setup_rgb_light_layer() in townk_layers.c for initialization of the RGB
 *      layer system.
 */
void keyboard_post_init_user(void) {
    /**
     * @var struct saved_values::left_scroll
     * @brief Enables/disables scroll mode for the left trackball/pointing
     *        device.
     *
     * When enabled, moving the left trackball scrolls instead of moving the
     * cursor.
     */
    global_saved_values.left_scroll = true;

    /**
     * @var struct saved_values::right_scroll
     * @brief Enables/disables scroll mode for the right trackball/pointing
     *        device.
     *
     * When enabled, moving the right trackball scrolls instead of moving the
     * cursor.
     */
    global_saved_values.right_scroll = false;

    /**
     * @var struct saved_values::auto_mouse
     * @brief Enables/disables automatic mouse layer activation.
     *
     * When enabled, the keyboard automatically switches to mouse layer when
     * pointer device is used.
     */
    global_saved_values.auto_mouse = true;

    /**
     * @var struct saved_values::left_dpi_index
     * @brief Index into the DPI (dots per inch) choices array
     *        (`DPI_CHOICES_LENGTH`) for the left trackball.
     *
     * It controls the sensitivity/speed of the left pointing device.
     *
     * Valid values:
     *
     * - MOUSE_DPI_200 (0)
     * - MOUSE_DPI_400 (1)
     * - MOUSE_DPI_800 (2)
     * - MOUSE_DPI_1200 (3)
     * - MOUSE_DPI_1600 (4)
     * - MOUSE_DPI_2400 (5)
     */
    global_saved_values.left_dpi_index = MOUSE_DPI_400;

    /**
     * @var struct saved_values::right_dpi_index
     * @brief Index into the DPI (dots per inch) choices array
     *        (`DPI_CHOICES_LENGTH`) for the right trackball
     *
     * It controls the sensitivity/speed of the right pointing device.
     *
     * Valid values:
     *
     * - MOUSE_DPI_200 (0)
     * - MOUSE_DPI_400 (1)
     * - MOUSE_DPI_800 (2)
     * - MOUSE_DPI_1200 (3)
     * - MOUSE_DPI_1600 (4)
     * - MOUSE_DPI_2400 (5)
     */
    global_saved_values.right_dpi_index = MOUSE_DPI_1200;

    /**
     * @var struct saved_values::mh_timer_index
     * @brief Index into the mouse-hold timer choices array
     *        (`mh_timer_choices`)
     *
     * Controls auto-exit timeout for the mouse mode. Determines how long the
     * mouse layer stays active after last mouse movement.
     *
     * Valid values:
     *
     * - MOUSE_LAYER_TIMEOUT_200_MS (0)
     * - MOUSE_LAYER_TIMEOUT_300_MS (1)
     * - MOUSE_LAYER_TIMEOUT_400_MS (2)
     * - MOUSE_LAYER_TIMEOUT_500_MS (3)
     * - MOUSE_LAYER_TIMEOUT_800_MS (4)
     * - MOUSE_LAYER_TIMEOUT_NONE (5)
     */
    global_saved_values.mh_timer_index = MOUSE_LAYER_TIMEOUT_NONE;

    setup_rgb_light_layer();
    setup_dynamic_keymap();
}

/**
 * @brief User-level key event processing hook
 *
 * This QMK hook is called for every key press and release event before the
 * default QMK key processing occurs. It allows for custom key behavior by
 * intercepting and potentially handling keys before they reach the standard
 * QMK processing pipeline.
 *
 * The function currently delegates all key processing to the special mouse
 * keys handler, which implements dual-function modifier/mouse button behavior
 * for certain keys (MB_SFT, MB_ALT, MB_GUI, MB_CTL).
 *
 * **Processing Flow:**
 * 1. Check if the key is a special mouse button key.
 * 2. If handled by special mouse keys, stop further processing.
 * 3. Otherwise, allow standard QMK processing to continue.
 *
 * @param keycode The keycode that was pressed or released.
 * @param record Pointer to the key event record containing:
 *               - event.pressed: true on press, false on release.
 *               - event.key: the matrix position of the key.
 *               - event.time: timestamp of the event.
 *
 * @return true to continue processing this key with QMK defaults, false to
 *         stop processing (key was fully handled).
 *
 * @note This is a QMK user-level hook that gets called automatically by the
 *       firmware for every key event.
 *
 * @note The content of this function is not all the work done when keys are
 *       pressed and released. Modules in this userspace can add their own
 *       behavior that is controlled by the QMK firmware automatically.
 *
 * @see process_special_mouse_keys() in townk_mouse.c for special key handling.
 */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == KC_ESC) {
        mouse_mode(false);
    }
    if (!process_special_mouse_keys(keycode, record)) {
        return false;
    }
    return true;
}
