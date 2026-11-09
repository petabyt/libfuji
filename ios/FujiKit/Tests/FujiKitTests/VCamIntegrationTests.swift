import Foundation
import Testing
@testable import FujiKit

/// End-to-end through PTPClient -> CFujiCore -> libfuji/libpict against
/// petabyt/vcam (emulated Fujifilm camera). Opt-in:
///   FUJIKIT_VCAM_CARD=<card dir served by vcam> swift test --filter VCam
/// with vcam running: vcam fuji_x_s10 tcp --ip 127.0.0.1 --fs <card dir>
/// This validates the protocol plumbing, not X-T30 III behaviour.
@Suite(.enabled(if: ProcessInfo.processInfo.environment["FUJIKIT_VCAM_CARD"] != nil), .serialized)
struct VCamIntegrationTests {
	let card = ProcessInfo.processInfo.environment["FUJIKIT_VCAM_CARD"] ?? ""
	let host = ProcessInfo.processInfo.environment["FUJIKIT_VCAM_HOST"] ?? "127.0.0.1"

	func findOnCard(_ name: String) -> URL? {
		let e = FileManager.default.enumerator(at: URL(fileURLWithPath: card), includingPropertiesForKeys: nil)
		while let url = e?.nextObject() as? URL {
			if url.lastPathComponent.caseInsensitiveCompare(name) == .orderedSame { return url }
		}
		return nil
	}

	@Test func browseDownloadAndInspect() async throws {
		let logger = FujiLogger(capacity: 2000)
		let client = try PTPClient(transport: .wirelessComm, logger: logger)
		try await client.connect(ip: host, requireCameraSubnet: false)
		try await client.setup(clientName: "CamBridge-SwiftTest", timeout: .seconds(20))

		let state = try await client.state()
		#expect(state.numObjects > 0)
		#expect(state.cameraName.isEmpty == false)
		let caps = CameraCapabilities.from(state)
		#expect(caps.gallery.isAvailable)
		#expect(caps.download.isAvailable)

		var biggest: (UInt32, CameraObjectInfo)?
		for h in try await client.objectHandles() {
			guard let info = try? await client.objectInfo(handle: h), info.format == .jpeg else { continue }
			if info.compressedSize > (biggest?.1.compressedSize ?? 0) { biggest = (h, info) }
		}
		let (handle, info) = try #require(biggest)

		let thumb = try await client.thumbnail(handle: handle)
		#expect(thumb.jpeg.map { MediaFormat.sniff([UInt8]($0.prefix(4))) } == .jpeg)

		let env = try TempEnv(); defer { env.cleanup() }
		let request = TransferRequest(identity: ObjectIdentity(cameraID: "vcam", info: info), lastKnownHandle: handle,
									  cameraDisplayName: state.cameraName, format: info.format, dateCreated: info.dateCreated)
		var options = TransferOptions()
		options.auditProperties = true
		let out = try await env.engine.run(request, source: client, options: options) { _, _ in }
		let reference = try #require(findOnCard(info.filename))
		#expect(try Data(contentsOf: out.fileURL!) == Data(contentsOf: reference))
		#expect(out.integrity == .ok)

		// Resume path against the real C download (start offset) and offset reads
		let partial = try await client.readPartial(handle: handle, offset: 1000, length: 4096)
		#expect(Data(partial) == (try Data(contentsOf: reference)).subdata(in: 1000..<5096))

		if let di = try? await client.deviceInfo() {
			#expect(di.manufacturer == "FUJIFILM")
		}
		// Probing must never crash, whatever descriptor forms the camera returns
		for p in KnownProperty.all {
			_ = try? await client.propertyDescriptor(p.code)
		}
		#expect(await client.isConnected())
		await client.close()
	}
}
