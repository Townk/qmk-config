/* Host-test stand-in for QMK's quantum/action.h.
 *
 * townk_mouse.h includes <action.h> for keyrecord_t. Under test that type is
 * already provided by sm_td's own harness shim (sm_td_bindings.c), which the
 * fixture includes first -- so this header only has to exist and stay empty.
 * It is never compiled into firmware; the real action.h is used on-device.
 */
#pragma once
