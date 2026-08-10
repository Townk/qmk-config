/* Host-test stand-in for QMK's quantum/caps_word.h. Never in firmware.
 * All three are defined by sm_td's shim (0.5.6+), which models Caps Word
 * state for real; declared here for townk_smtd.c. */
#pragma once

#include <stdbool.h>

void caps_word_on(void);
void caps_word_off(void);
