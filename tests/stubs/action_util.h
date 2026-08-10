/* Host-test stand-in for QMK's quantum/action_util.h. Never in firmware.
 * get_mods/register_mods/unregister_mods/send_keyboard_report come from sm_td's
 * shim; the one-shot and weak-mod accessors do not. */
#pragma once

#include <stdint.h>

void    set_oneshot_mods(uint8_t mods);
uint8_t get_oneshot_mods(void);
void    clear_oneshot_mods(void);
uint8_t get_weak_mods(void);
