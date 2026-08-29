/* Host-test stand-in for QMK's quantum/modifiers.h. Never in firmware. */
#pragma once

#include "keycodes.h"

/* sm_td's harness shim defines MOD_BIT already; match it rather than fight it. */
#ifndef MOD_BIT
#    define MOD_BIT(code) (1 << ((code) & 0x07))
#endif

#define MOD_LSFT 0x02

#define MOD_BIT_LSHIFT MOD_BIT(KC_LSFT)
#define MOD_BIT_RSHIFT MOD_BIT(KC_RSFT)
#define MOD_MASK_SHIFT (MOD_BIT_LSHIFT | MOD_BIT_RSHIFT)

#define MOD_BIT_LCTRL MOD_BIT(KC_LCTL)
#define MOD_BIT_RCTRL MOD_BIT(KC_RCTL)
#define MOD_BIT_LALT MOD_BIT(KC_LALT)
#define MOD_BIT_RALT MOD_BIT(KC_RALT)
#define MOD_BIT_LGUI MOD_BIT(KC_LGUI)
#define MOD_BIT_RGUI MOD_BIT(KC_RGUI)
#define MOD_MASK_CTRL (MOD_BIT_LCTRL | MOD_BIT_RCTRL)
#define MOD_MASK_ALT (MOD_BIT_LALT | MOD_BIT_RALT)
#define MOD_MASK_GUI (MOD_BIT_LGUI | MOD_BIT_RGUI)
