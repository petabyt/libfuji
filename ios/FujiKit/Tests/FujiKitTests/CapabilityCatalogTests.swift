import Foundation
import Testing
@testable import FujiKit

@Suite struct CapabilityCatalogTests {
	@Test func noFeatureClaimsDeviceVerification() {
		// Nothing has been run on an X-T30 III yet; this must be updated deliberately.
		#expect(!CapabilityCatalog.entries.contains { $0.statuses.contains(.deviceVerified) })
		#expect(Set(CapabilityCatalog.entries.map(\.feature)).count == CapabilityCatalog.entries.count)
	}

	@Test func renderMarkdown() throws {
		let md = CapabilityCatalog.markdown()
		#expect(md.contains("| 蓝牙 | 扫描识别相机 |"))
		if let path = ProcessInfo.processInfo.environment["FUJIKIT_WRITE_MATRIX"] {
			try md.write(toFile: path, atomically: true, encoding: .utf8)
		}
	}
}
