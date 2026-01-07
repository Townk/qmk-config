#import "svalboard-keys.typ": *

#set page(
    margin: 0pt,
    width: 1600pt, height: 560pt,
    fill: none,
    background: rect(
        width: 1598pt,
        height: 558pt,
        radius: 1cm,
        fill: white,
        stroke: (paint: black, thickness: 2pt),
    ),
)

#let keymap = yaml("keymap.yaml")

#let keymapWidth = 1540pt // 1540pt x 500pt
#let keymapHeight = keymapWidth * 0.3246

#block(inset: 30pt)[
  #for layer in keymap.layers {
    layerKeymap(keymap, layer, size: (keymapWidth, keymapHeight))
  }
]
