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

/**
 * @file townk_layers.c
 * @brief RGB lighting layer indicators for keyboard layers
 *
 * This file implements RGB lighting indicators that visually represent the
 * currently active keyboard layer. Each layer is assigned a unique color that
 * is displayed on the keyboard's RGB LEDs when that layer is active.
 *
 * **Layer Color Mapping:**
 * - Layer 0 (_BASE): Green
 * - Layer 1 (_QWERTY): Blue
 * - Layer 2 (_GAM1): Turquoise
 * - Layer 3 (_NAV): Orange
 * - Layer 4 (_NUM): Azure
 * - Layer 5 (_SYM): Coral
 * - Layer 6 (_FUN): Purple
 * - Layer 7 (_MED): Yellow
 * - Layer 8: Pink
 * - Layer 9: Charteruse
 * - Layer 10: Gold
 * - Layer 11: Teal
 * - Layer 12: Spring Green
 * - Layer 13: Goldenrod
 * - Layer 14 (_SYS): Red
 * - Layer 15 (_MBO): Magenta
 *
 * @author Thiago Alves
 * @date 2025
 */

#include "townk_layers.h"
#include "rgblight.h"
#include "color.h"

// HSV version of the layer colors
#define BASE_GREEN          70, 220, 180
#define QWT_TEAL           105, 240, 150
#define GAME_1_BLUE        153, 255, 180
#define GAME_2_BLUE        137, 255, 180
#define NAV_ORANGE          20, 255, 180
#define NUM_BLUE           143, 255, 180
#define FUN_PURPLE         183, 255, 180
#define MED_YELLOW          43, 255, 180
#define SYS_RED              0, 255, 180
#define MBO_MAGENTA        208, 255, 180

extern void mouse_mode(bool on);

#define LAYER_COLOR(name, color) rgblight_segment_t const (name)[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, color})
LAYER_COLOR(layer0_colors, BASE_GREEN);    // Base layer   (#347156)
LAYER_COLOR(layer1_colors, QWT_TEAL);      // Base QWERTY  (#2F898B)
LAYER_COLOR(layer2_colors, GAME_1_BLUE);   // Game layer 1 (#355A93)
LAYER_COLOR(layer3_colors, GAME_2_BLUE);   // Game layer 2 (#0C7595)
LAYER_COLOR(layer4_colors, NAV_ORANGE);    // Navigation   (#D96702)
LAYER_COLOR(layer5_colors, NUM_BLUE);      // Numbers      (#41687E)
LAYER_COLOR(layer6_colors, HSV_CORAL);     // Symbols      (#FF7C4D)
LAYER_COLOR(layer7_colors, FUN_PURPLE);    // Fn Keys      (#605181)
LAYER_COLOR(layer8_colors, MED_YELLOW);    // Multi-Media  (#767709)
LAYER_COLOR(layer9_colors, HSV_PINK);
LAYER_COLOR(layer10_colors, HSV_CHARTREUSE);
LAYER_COLOR(layer11_colors, HSV_GOLD);
LAYER_COLOR(layer12_colors, HSV_TEAL);
LAYER_COLOR(layer13_colors, HSV_SPRINGGREEN);
LAYER_COLOR(layer14_colors, SYS_RED);     // System      (#9C2927)
LAYER_COLOR(layer15_colors, MBO_MAGENTA); // Mouse layer (#8C3396)
#undef LAYER_COLOR

/**
 * @brief Array of RGB lighting segments for all keyboard layers
 *
 * This array contains pointers to RGB lighting segment definitions for each
 * keyboard layer (0-15). Each element corresponds to a layer index and defines
 * the color to be displayed when that layer is active.
 *
 * The array is used by QMK's RGB lighting system to automatically update LED
 * colors based on the current layer state.
 *
 * @see setup_rgb_light_layer() to initialize the RGB layer system.
 * @see layer_state_set_user() for automatic layer color updates.
 */
const rgblight_segment_t*  const rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    layer0_colors, layer1_colors, layer2_colors, layer3_colors,
    layer4_colors, layer5_colors, layer6_colors, layer7_colors,
    layer8_colors, layer9_colors, layer10_colors, layer11_colors,
    layer12_colors, layer13_colors, layer14_colors, layer15_colors
);

/**
 * @brief User callback for default layer state changes
 *
 * This QMK hook is called whenever the default layer state changes. It updates
 * the RGB lighting to reflect whether layer 0 (base layer) is active as the
 * default layer.
 *
 * The function enables the layer 0 RGB lighting segment when layer 0 is the
 * default layer, and disables it otherwise.
 *
 * @param state The new default layer state bitmask.
 * @return layer_state_t The same state value (required by QMK).
 *
 * @note This is a QMK user-level hook that gets called automatically by the
 *       firmware when the default layer changes.
 */
layer_state_t default_layer_state_set_user(layer_state_t state) {
  rgblight_set_layer_state(0, layer_state_cmp(state, 0));
  return state;
}

/**
 * @brief User callback for layer state changes
 *
 * This QMK hook is called whenever any layer is activated or deactivated. It
 * updates the RGB lighting for all layers to reflect the current layer state.
 *
 * The function iterates through all available RGB lighting layers (0-15) and
 * enables/disables each layer's RGB segment based on whether that layer is
 * currently active in the layer state.
 *
 * When multiple layers are active simultaneously (e.g., momentary layer on top
 * of base layer), the RGB lighting will show the highest active layer's color.
 *
 * @param state The new layer state bitmask.
 * @return layer_state_t The same state value (required by QMK).
 *
 * @note This is a QMK user-level hook that gets called automatically by the
 *       firmware whenever the layer state changes.
 *
 * @see RGBLIGHT_LAYERS constant for the maximum number of RGB layers.
 */
layer_state_t layer_state_set_user(layer_state_t state) {
  for (int i = 0; i < RGBLIGHT_LAYERS; ++i) {
      rgblight_set_layer_state(i, layer_state_cmp(state, i));
  }

  // Disable auto-mouse when on the game layer
  if (layer_state_cmp(state, _GAM1) || layer_state_cmp(state, _GAM2)) {
      global_saved_values.auto_mouse = false;
      mouse_mode(false);
  } else {
      global_saved_values.auto_mouse = true;
  }

  return state;
}

void setup_rgb_light_layer() {
    rgblight_layers = rgb_layers;
}

