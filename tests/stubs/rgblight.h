/* Host-test stand-in for QMK's quantum/rgblight/rgblight.h.
 *
 * townk_layers.h includes it for rgblight_segment_t, which the layer enum
 * declaration needs in scope. Nothing under test touches RGB, so an opaque
 * type is enough. Never compiled into firmware.
 */
#pragma once

typedef struct {
    unsigned char index;
    unsigned char count;
    unsigned char hue;
    unsigned char sat;
    unsigned char val;
} rgblight_segment_t;
