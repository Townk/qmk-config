// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "svalwatch",
    platforms: [.macOS(.v13)],
    targets: [
        // Everything that can be decided without a board: packet layout,
        // reply parsing, change detection, pacing, the output format.
        .target(name: "SvalWatchCore"),
        // The IOKit side: finding the board, talking to it, sleep/lock.
        .executableTarget(name: "svalwatch", dependencies: ["SvalWatchCore"]),
        .testTarget(name: "SvalWatchCoreTests", dependencies: ["SvalWatchCore"]),
    ]
)
