#import "svalboard-keys.typ": *

#let pageWidth = 1800pt
#let pageBorderWidth = pageWidth - 2pt
#let pageHeight = 2150pt
#let pageBorderHeight = pageHeight - 2pt

#set page(
    margin: 0pt,
    width: pageWidth, height: pageHeight,
    fill: none,
    background: rect(
        width: pageBorderWidth,
        height: pageBorderHeight,
        radius: 1cm,
        fill: white,
        stroke: (paint: black, thickness: 2pt),
    ),
)

#let keymap = yaml("keymap.yaml")

#let keymapWidth = 1350pt // 1540pt x 500pt

#let tableRows = ()
#for i in range(7, -1, step: -1) {
  let layer = keymap.layers.at(i)
  let layerName = if i > 0 { layer.name } else { "LETTERS" }
  let layerSubName = if i > 0 { none } else { layer.name }
  let layerColor = layerPrimaryColor(layer)
  let vertInset = if i == 7 { 21pt } else { 50pt }
  tableRows += (
    box(inset: (top: vertInset), width: 370pt)[
      #if i == 7 {
        box(inset: (left: 15pt),
          text(
            font: "Roboto", weight: "black", fill: gray.darken(30%),
            tracking: 2pt, size: 22pt,
            "LAYERS"
          )
        )
      }
      #rect(
        inset: 15pt,
        width: 270pt,
        radius: 5pt,
        fill: layerColor,
        text(
          font: "Roboto", weight: "black", fill: white,
          tracking: 2pt, size: 22pt,
          upper[#(i+1) #layerName]
        )
      )
      #if layerSubName != none {
        box(inset: (left: 15pt),
          text(
            font: "Roboto", weight: "black", fill: layerColor,
            tracking: 2pt, size: 22pt,
            upper(layerSubName)
          )
        )
      }
    ], minimalLayer(keymap, layer, keymapWidth)
  )
}


#block(inset: 30pt)[
  #table(
    columns: 2,
    row-gutter: 20pt,
    stroke: none,
    image(width: 300pt, "./images/svalboard-logo.png"),
    box(inset: (top: 37pt),
      text(
        font: "Roboto", weight: "black", fill: gray.darken(70%),
        size: 32pt,
        "Colemak Layers Layout"
      )
    ),
    ..tableRows,
    box(inset: (top: 50pt), width: 370pt)[
      #rect(
        inset: 15pt,
        width: 270pt,
        radius: 5pt,
        fill: black,
        text(
          font: "Roboto", weight: "black", fill: white,
          tracking: 2pt, size: 22pt,
          upper[THUMBS]
        )
      )
    ], align(center)[
      #thumb-cluster(
        keymap, keymap.layers.at(0), TCL,
        size: (keymapWidth * 0.2, keymapWidth * 0.13)
      )#h(55pt)#thumb-cluster(
        keymap, keymap.layers.at(0), TCR,
        size: (keymapWidth * 0.2, keymapWidth * 0.13)
      )
    ]
  )
]

