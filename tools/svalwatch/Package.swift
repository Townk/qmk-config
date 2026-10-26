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
        .target(name: "SvalWatchDevice", dependencies: ["SvalWatchCore"]),
        // JSON lines on stdout, for anything that wants the raw stream.
        .executableTarget(name: "svalwatch", dependencies: ["SvalWatchCore", "SvalWatchDevice"]),
        // The always-on layer overlay window.
        .executableTarget(name: "svaloverlay", dependencies: ["SvalWatchCore", "SvalWatchDevice"]),
        .testTarget(name: "SvalWatchCoreTests", dependencies: ["SvalWatchCore"]),
    ]
)
