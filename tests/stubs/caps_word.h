/* Host-test stand-in for QMK's quantum/caps_word.h. Never in firmware.
 * is_caps_word_on() comes from sm_td's shim; these two do not. */
#pragma once

#include <stdbool.h>

void caps_word_on(void);
void caps_word_off(void);
