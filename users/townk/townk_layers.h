/* Copyright (C) 2024 Thiago Alves (https://github.com/townk)
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
    _BASE,
    _NAV,
    _NUM,
    _SYM,
    _FUN,
    _SYS = LAST_LAYER - 1,
    _MBO = LAST_LAYER,
};

void setup_rgb_light_layer(void);

#endif // QMK_USERSPACE_TOWNK_LAYERS_H
