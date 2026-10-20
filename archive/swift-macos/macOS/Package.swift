// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "ScreenEncoder",
    platforms: [.macOS(.v13)],
    products: [.executable(name: "ScreenEncoder", targets: ["ScreenEncoder"])],
    targets: [
        .target(name: "ScreenEncoderCore"),
        .executableTarget(name: "ScreenEncoder", dependencies: ["ScreenEncoderCore"]),
        // Executable checks also run with Command Line Tools (XCTest requires full Xcode).
        .executableTarget(name: "ScreenEncoderChecks", dependencies: ["ScreenEncoderCore"], path: "Checks")
    ]
)
