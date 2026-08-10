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
