// swift-tools-version: 6.0
import PackageDescription

let package = Package(
	name: "FujiKit",
	platforms: [
		.iOS("27.0"),
		// macOS only so the platform-neutral parts and tests can run with `swift test` on a Mac.
		.macOS("15.0"),
	],
	products: [
		.library(name: "FujiKit", targets: ["FujiKit"]),
	],
	targets: [
		// Vendored libfuji/libpict/fp (see ios/scripts/vendor-sync.sh) + the iOS shim.
		.target(
			name: "CFujiCore",
			path: "Sources/CFujiCore",
			exclude: [
				"upstream/VENDOR_MANIFEST.txt",
				"upstream/libfuji/LICENSE",
				"upstream/libpict/LICENSE",
			],
			sources: ["upstream", "shim"],
			publicHeadersPath: "include",
			cSettings: [
				.headerSearchPath("upstream/libpict"),
				.headerSearchPath("upstream/libfuji"),
				.headerSearchPath("upstream/fp"),
				.define("EZXML_NOMMAP"),
			]
		),
		.target(
			name: "FujiKit",
			dependencies: ["CFujiCore"],
			path: "Sources/FujiKit",
			swiftSettings: [
				.swiftLanguageMode(.v6),
			]
		),
		.testTarget(
			name: "FujiKitTests",
			dependencies: ["FujiKit"],
			path: "Tests/FujiKitTests",
			resources: [.copy("Fixtures")],
			swiftSettings: [
				.swiftLanguageMode(.v6),
			]
		),
	]
)
