/* Host-test stand-in for QMK's quantum/rgblight/rgblight.h.
 *
 * townk_layers.h includes it for rgblight_segment_t, which the layer enum
 * declaration needs in scope, and townk_layers.c uses the layer-table
 * macros and rgblight_set_layer_state(). Nothing under test reads the RGB
 * state, so the segment type is real enough to initialize and everything
 * else is the thinnest declaration that links. Never compiled into
 * firmware.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    unsigned char index;
    unsigned char count;
    unsigned char hue;
    unsigned char sat;
    unsigned char val;
} rgblight_segment_t;

#ifndef RGBLIGHT_LAYERS
#    define RGBLIGHT_LAYERS 16
#endif

/* QMK's macros append an end-marker segment / a NULL terminator; the shapes
 * here match, so the initializers in townk_layers.c compile unchanged. */
#define RGBLIGHT_LAYER_SEGMENTS(...) \
    { __VA_ARGS__, {255, 0, 0, 0, 0} }
#define RGBLIGHT_LAYERS_LIST(...) \
    { __VA_ARGS__, 0 }

extern const rgblight_segment_t *const *rgblight_layers;

void rgblight_set_layer_state(uint8_t layer, bool enabled);
