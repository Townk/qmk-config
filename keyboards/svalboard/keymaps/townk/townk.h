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

#ifndef QMK_USERSPACE_TOWNK_SVALBOARD_TOWNK_H
#define QMK_USERSPACE_TOWNK_SVALBOARD_TOWNK_H

#include "townk_keycodes.h"
#include "townk_layers.h"

// helper macros to configure the Auto-Mouse layer
#define MOUSE_DPI_200 0
#define MOUSE_DPI_400 1
#define MOUSE_DPI_800 2
#define MOUSE_DPI_1200 3
#define MOUSE_DPI_1600 4
#define MOUSE_DPI_2400 5
#define MOUSE_LAYER_TIMEOUT_300_MS 0
#define MOUSE_LAYER_TIMEOUT_500_MS 1
#define MOUSE_LAYER_TIMEOUT_800_MS 2
#define MOUSE_LAYER_TIMEOUT_NONE 3

#endif // QMK_USERSPACE_TOWNK_SVALBOARD_TOWNK_H
