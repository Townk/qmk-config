/* Host-test stand-in for QMK's quantum/color.h.
 *
 * townk_layers.c includes it for the HSV_* named colors used in the RGB
 * layer table. Only the names the file actually uses are provided; the
 * values match QMK's, though nothing under test reads them. Never compiled
 * into firmware.
 */
#pragma once

#define HSV_CORAL 11, 176, 255
#define HSV_PINK 234, 128, 255
#define HSV_CHARTREUSE 64, 255, 255
#define HSV_GOLD 36, 255, 255
#define HSV_TEAL 128, 255, 128
#define HSV_SPRINGGREEN 106, 255, 255
