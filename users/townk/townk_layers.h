/* Copyright (C) 2025 Thiago Alves (https://github.com/townk)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QMK_USERSPACE_TOWNK_LAYERS_H
#define QMK_USERSPACE_TOWNK_LAYERS_H

#include "rgblight.h"

#ifdef SVALBOARD
#    include "keymap_support.h"
#    define LAST_LAYER MH_AUTO_BUTTONS_LAYER
#else
#    define LAST_LAYER 6
#endif

extern const rgblight_segment_t* const rgb_layers[];

enum layer_names {
    _BASE,                 ///< The default keyboard layer.
    _NAV,                  ///< Navigation layer. Has keys used to move around the OS.
    _NUM,                  ///< Numbers layer. Has all digits and some math operations.
    _SYM,                  ///< Symbols layer. Has all symbols organized ergonomically.
    _FUN,                  ///< Function keys layer. Has all `F?` keys available.
    _MED,                  ///< Media layer. Has keys to control muiti-media in the OS.
    _SYS = LAST_LAYER - 1, ///< Svalboard specific keys layer.
    _MBO = LAST_LAYER,     ///< Mouse buttons layer.
};

/**
 * @brief Initialize RGB lighting layer indicators
 *
 * This function sets up the RGB lighting system to use layer-based color
 * indicators. It must be called during keyboard initialization (typically from
 * keyboard_post_init_user()) to register the rgb_layers array with QMK's RGB
 * lighting subsystem.
 *
 * After this function is called, the RGB LEDs will automatically change colors
 * based on the active layer, as defined in the rgb_layers array.
 *
 * @note This function should be called once during keyboard initialization.
 *
 * @see keyboard_post_init_user() in keymap.c where this function is called.
 * @see rgb_layers array for the layer color definitions.
 */
void setup_rgb_light_layer(void);

#endif // QMK_USERSPACE_TOWNK_LAYERS_H
