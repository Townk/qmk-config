/* Host-test stand-in for QMK's platforms/timer.h. Never in firmware.
 * Both functions are defined by sm_td's shim, whose virtual clock (0.6.4+)
 * stands still until a test advances it with TEST_advance_time(). */
#pragma once

#include <stdint.h>

uint32_t timer_read32(void);
uint32_t timer_elapsed32(uint32_t last);
