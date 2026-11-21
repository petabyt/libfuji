import Foundation
import Testing
@testable import FujiKit

/// In-memory camera implementing the same contract as PTPClient.
final class FakeCamera: CameraObjectSource, @unchecked Sendable {
	struct Object { var info: CameraObjectInfo; var bytes: [UInt8] }
	private let lock = NSLock()
	var objects: [UInt32: Object] = [:]
	/// Fail with an I/O error once this many bytes have been delivered in one download.
	var failAfterBytes: UInt64?
	var offsetReadsSupported = true
	var corruptOffsetReads = false
	var downloads: [(handle: UInt32, offset: UInt64)] = []

	static func jpeg(size: Int, seed: UInt8) -> [UInt8] {
		var b = [UInt8](repeating: 0, count: size)
		for i in b.indices { b[i] = UInt8(truncatingIfNeeded: i &* 7 &+ Int(seed)) }
		b[0] = 0xFF; b[1] = 0xD8; b[2] = 0xFF
		b[size - 2] = 0xFF; b[size - 1] = 0xD9
		return b
	}

	func add(handle: UInt32, name: String, bytes: [UInt8], date: String = "20251123T104512") {
		objects[handle] = Object(info: CameraObjectInfo(objectFormat: 0x3801, compressedSize: UInt32(bytes.count), filename: name, dateCreatedRaw: date), bytes: bytes)
	}

	func objectHandles() async throws -> [UInt32] { lock.withLock { objects.keys.sorted() } }

	func objectInfo(handle: UInt32) async throws -> CameraObjectInfo {
		guard let o = lock.withLock({ objects[handle] }) else {
			throw PTPError.core(code: CoreRC.checkCode, responseCode: 0x2009, operation: "info")
		}
		return o.info
	}

	func download(handle: UInt32, from offset: UInt64, audit: Bool, sink: any DownloadSink) async throws -> DownloadReport {
		guard let o = lock.withLock({ objects[handle] }) else {
			throw PTPError.core(code: CoreRC.checkCode, responseCode: 0x2009, operation: "download")
		}
		lock.withLock { downloads.append((handle, offset)) }
		guard sink.begin(info: o.info, startOffset: offset) else {
			throw PTPError.core(code: CoreRC.canceled, responseCode: 0, operation: "download")
		}
		var pos = Int(offset)
		var delivered: UInt64 = 0
		while pos < o.bytes.count {
			try Task.checkCancellation()
			if let limit = failAfterBytes, delivered >= limit {
				failAfterBytes = nil
				throw PTPError.core(code: CoreRC.io, responseCode: 0, operation: "download")
			}
			let n = min(300_000, o.bytes.count - pos)
			let ok = o.bytes[pos..<(pos + n)].withUnsafeBytes { sink.write($0, at: UInt64(pos), total: UInt64(o.bytes.count)) }
			guard ok else { throw PTPError.core(code: CoreRC.canceled, responseCode: 0, operation: "download") }
			pos += n
			delivered += UInt64(n)
		}
		return DownloadReport(expectedSize: UInt64(o.bytes.count), bytesReceived: UInt64(pos))
	}

	func readPartial(handle: UInt32, offset: UInt32, length: UInt32) async throws -> [UInt8] {
		guard offsetReadsSupported else { throw PTPError.core(code: CoreRC.checkCode, responseCode: 0x2005, operation: "partial") }
		guard let o = lock.withLock({ objects[handle] }) else { throw PTPError.core(code: CoreRC.checkCode, responseCode: 0x2009, operation: "partial") }
		var slice = Array(o.bytes[Int(offset)..<min(o.bytes.count, Int(offset + length))])
		if corruptOffsetReads, !slice.isEmpty { slice[0] ^= 0xFF }
		return slice
	}
}

struct TempEnv {
	let root: URL
	let paths: AppPaths
	let store: ImportStore
	let engine: TransferEngine

	init() throws {
		root = FileManager.default.temporaryDirectory.appendingPathComponent("fujikit-\(UUID().uuidString)")
		paths = AppPaths(documents: root.appendingPathComponent("Documents"), applicationSupport: root.appendingPathComponent("Support"),
						 caches: root.appendingPathComponent("Caches"))
		try paths.ensureDirectories()
		store = ImportStore(fileURL: paths.importRecordsFile)
		engine = TransferEngine(paths: paths, store: store, logger: FujiLogger(capacity: 100))
	}

	func cleanup() { try? FileManager.default.removeItem(at: root) }

	func request(_ cam: FakeCamera, handle: UInt32) -> TransferRequest {
		let info = cam.objects[handle]!.info
		return TransferRequest(identity: ObjectIdentity(cameraID: "cam1", info: info), lastKnownHandle: handle,
							   cameraDisplayName: "X-T30 III", format: info.format, dateCreated: info.dateCreated)
	}
}

@Suite struct TransferEngineTests {
	@Test func downloadsVerifiesAndRecords() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		let bytes = FakeCamera.jpeg(size: 1_234_567, seed: 1)
		cam.add(handle: 1, name: "DSCF0001.JPG", bytes: bytes)
		let progress = Locked<UInt64>(0)
		let out = try await env.engine.run(env.request(cam, handle: 1), source: cam, options: TransferOptions()) { r, _ in progress.set(r) }
		guard case .imported(let rec) = out.kind else { Issue.record("expected import"); return }
		#expect(progress.value == UInt64(bytes.count))
		#expect(try Data(contentsOf: out.fileURL!) == Data(bytes))
		#expect(rec.sha256 == SHA256Hasher.hex(of: Data(bytes)))
		#expect(out.fileURL!.path.contains("导入/X-T30 III/2025-11-23/DSCF0001.JPG"))
		#expect(env.store.status(for: rec.identity) == .alreadyImported(rec))
	}

	@Test func identicalBytesAreDuplicates() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		let bytes = FakeCamera.jpeg(size: 50_000, seed: 2)
		cam.add(handle: 1, name: "DSCF0001.JPG", bytes: bytes)
		cam.add(handle: 2, name: "COPY0001.JPG", bytes: bytes, date: "20251124T000000")
		_ = try await env.engine.run(env.request(cam, handle: 1), source: cam, options: TransferOptions()) { _, _ in }
		let second = try await env.engine.run(env.request(cam, handle: 2), source: cam, options: TransferOptions()) { _, _ in }
		guard case .duplicate = second.kind else { Issue.record("expected duplicate"); return }
		#expect(env.store.records.count == 1)
	}

	@Test func recycledNameWithDifferentContentKeepsBoth() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		cam.add(handle: 1, name: "DSCF0001.JPG", bytes: FakeCamera.jpeg(size: 40_000, seed: 3))
		_ = try await env.engine.run(env.request(cam, handle: 1), source: cam, options: TransferOptions()) { _, _ in }
		// New card, counter reset: same name, same day, different photo
		cam.add(handle: 1, name: "DSCF0001.JPG", bytes: FakeCamera.jpeg(size: 41_000, seed: 4), date: "20251123T200000")
		let out = try await env.engine.run(env.request(cam, handle: 1), source: cam, options: TransferOptions()) { _, _ in }
		#expect(out.fileURL!.lastPathComponent == "DSCF0001-1.JPG")
		#expect(env.store.records.count == 2)
	}

	@Test func findsObjectAfterHandlesShift() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		cam.add(handle: 1, name: "DSCF0001.JPG", bytes: FakeCamera.jpeg(size: 10_000, seed: 5))
		cam.add(handle: 2, name: "DSCF0002.JPG", bytes: FakeCamera.jpeg(size: 11_000, seed: 6))
		let req = env.request(cam, handle: 2)
		// Two new photos taken: listing order changes (newest first on older cameras)
		let shifted = cam.objects
		cam.objects = [:]
		cam.add(handle: 1, name: "DSCF0004.JPG", bytes: FakeCamera.jpeg(size: 12_000, seed: 7))
		cam.add(handle: 2, name: "DSCF0003.JPG", bytes: FakeCamera.jpeg(size: 13_000, seed: 8))
		cam.objects[3] = shifted[2]
		cam.objects[4] = shifted[1]
		let out = try await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		#expect(cam.downloads.last?.handle == 3)
		#expect(try Data(contentsOf: out.fileURL!) == Data(shifted[2]!.bytes))
	}

	@Test func missingObjectFails() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		cam.add(handle: 1, name: "DSCF0001.JPG", bytes: FakeCamera.jpeg(size: 10_000, seed: 9))
		let req = env.request(cam, handle: 1)
		cam.objects = [:]
		cam.add(handle: 1, name: "DSCF0009.JPG", bytes: FakeCamera.jpeg(size: 10_000, seed: 9))
		await #expect(throws: TransferFailure.objectNotFound) {
			_ = try await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		}
	}

	@Test func resumesOnlyAfterVerifiedOffsetRead() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		let bytes = FakeCamera.jpeg(size: 2_000_000, seed: 10)
		cam.add(handle: 1, name: "DSCF0010.JPG", bytes: bytes)
		let req = env.request(cam, handle: 1)
		cam.failAfterBytes = 900_000
		await #expect(throws: TransferFailure.self) {
			_ = try await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		}
		let out = try await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		#expect(out.resumedFrom == 900_000)
		#expect(cam.downloads.last?.offset == 900_000)
		#expect(try Data(contentsOf: out.fileURL!) == Data(bytes))
	}

	@Test func mismatchedResumeRestartsFromZero() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		let bytes = FakeCamera.jpeg(size: 2_000_000, seed: 11)
		cam.add(handle: 1, name: "DSCF0011.JPG", bytes: bytes)
		let req = env.request(cam, handle: 1)
		cam.failAfterBytes = 600_000
		_ = try? await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		cam.corruptOffsetReads = true
		let out = try await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		#expect(out.resumedFrom == 0)
		#expect(out.notes.contains { $0.contains("重新下载") })
		#expect(try Data(contentsOf: out.fileURL!) == Data(bytes))
	}

	@Test func cameraWithoutOffsetReadsRestarts() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		let bytes = FakeCamera.jpeg(size: 1_500_000, seed: 12)
		cam.add(handle: 1, name: "DSCF0012.JPG", bytes: bytes)
		let req = env.request(cam, handle: 1)
		cam.failAfterBytes = 600_000
		_ = try? await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		cam.offsetReadsSupported = false
		let out = try await env.engine.run(req, source: cam, options: TransferOptions()) { _, _ in }
		#expect(out.resumedFrom == 0)
	}

	@Test func truncatedCameraFileIsRejected() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		var bytes = FakeCamera.jpeg(size: 30_000, seed: 13)
		bytes[29_998] = 0; bytes[29_999] = 0 // no EOI: e.g. camera reported a wrong size
		cam.add(handle: 1, name: "DSCF0013.JPG", bytes: bytes)
		await #expect(throws: TransferFailure.self) {
			_ = try await env.engine.run(env.request(cam, handle: 1), source: cam, options: TransferOptions()) { _, _ in }
		}
		#expect(env.store.records.isEmpty)
	}
}

@Suite @MainActor struct TransferQueueTests {
	@Test func processesQueueAndSkipsKnownFiles() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		cam.add(handle: 1, name: "DSCF0001.JPG", bytes: FakeCamera.jpeg(size: 100_000, seed: 20))
		cam.add(handle: 2, name: "DSCF0002.JPG", bytes: FakeCamera.jpeg(size: 120_000, seed: 21))
		let queue = TransferQueue(engine: env.engine, fileURL: env.paths.transferQueueFile)
		queue.saveToPhotos = false
		queue.attach(source: cam)
		queue.enqueue([env.request(cam, handle: 1), env.request(cam, handle: 2)], cameraReduced: false)
		try await waitUntil { queue.items.allSatisfy { $0.state.isFinished } }
		#expect(queue.items.map(\.state) == [.completed, .completed])
		// Enqueue again: recognised from import records without downloading.
		queue.clearFinished()
		let before = cam.downloads.count
		queue.enqueue([env.request(cam, handle: 1)], cameraReduced: false)
		try await waitUntil { queue.items.allSatisfy { $0.state.isFinished } }
		#expect(queue.items.first?.state == .duplicate)
		#expect(cam.downloads.count == before)
	}

	@Test func connectionLossWaitsAndResumes() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = FakeCamera()
		let bytes = FakeCamera.jpeg(size: 1_000_000, seed: 22)
		cam.add(handle: 1, name: "DSCF0022.JPG", bytes: bytes)
		cam.failAfterBytes = 300_000
		let queue = TransferQueue(engine: env.engine, fileURL: env.paths.transferQueueFile)
		queue.saveToPhotos = false
		queue.attach(source: cam)
		queue.enqueue([env.request(cam, handle: 1)], cameraReduced: false)
		try await waitUntil { queue.items.first?.state == .waitingForConnection }
		#expect(!queue.hasSource)
		queue.attach(source: cam) // reconnected
		try await waitUntil { queue.items.first?.state == .completed }
		#expect(cam.downloads.last?.offset == 300_000)
		// A persisted queue reloads with in-flight items waiting for a connection.
		let reloaded = TransferQueue(engine: env.engine, fileURL: env.paths.transferQueueFile)
		#expect(reloaded.items.first?.state == .completed)
	}

	@Test func userCancelDiscardsPartial() async throws {
		let env = try TempEnv(); defer { env.cleanup() }
		let cam = SlowCamera()
		cam.add(handle: 1, name: "DSCF0030.JPG", bytes: FakeCamera.jpeg(size: 3_000_000, seed: 30))
		let queue = TransferQueue(engine: env.engine, fileURL: env.paths.transferQueueFile)
		queue.saveToPhotos = false
		queue.attach(source: cam)
		queue.enqueue([env.request(cam, handle: 1)], cameraReduced: false)
		try await waitUntil { queue.items.first?.state == .running && (queue.items.first?.bytesReceived ?? 0) > 0 }
		queue.cancel(queue.items[0].id)
		try await waitUntil { queue.items.first?.state == .cancelled }
		let partials = try FileManager.default.contentsOfDirectory(atPath: env.paths.partialDownloads.path)
		#expect(partials.isEmpty)
	}

	func waitUntil(timeout: Duration = .seconds(10), _ condition: @MainActor () -> Bool) async throws {
		let clock = ContinuousClock()
		let deadline = clock.now + timeout
		while !condition() {
			if clock.now > deadline { Issue.record("timed out"); return }
			try await Task.sleep(for: .milliseconds(20))
		}
	}
}

/// Delivers chunks slowly so a test can cancel mid-download.
final class SlowCamera: CameraObjectSource, @unchecked Sendable {
	let inner = FakeCamera()
	func add(handle: UInt32, name: String, bytes: [UInt8]) { inner.add(handle: handle, name: name, bytes: bytes) }
	var objects: [UInt32: FakeCamera.Object] { inner.objects }
	func objectHandles() async throws -> [UInt32] { try await inner.objectHandles() }
	func objectInfo(handle: UInt32) async throws -> CameraObjectInfo { try await inner.objectInfo(handle: handle) }
	func readPartial(handle: UInt32, offset: UInt32, length: UInt32) async throws -> [UInt8] { try await inner.readPartial(handle: handle, offset: offset, length: length) }
	func download(handle: UInt32, from offset: UInt64, audit: Bool, sink: any DownloadSink) async throws -> DownloadReport {
		let o = inner.objects[handle]!
		guard sink.begin(info: o.info, startOffset: offset) else { throw PTPError.core(code: CoreRC.canceled, responseCode: 0, operation: "") }
		var pos = Int(offset)
		while pos < o.bytes.count {
			if Task.isCancelled { throw PTPError.core(code: CoreRC.canceled, responseCode: 0, operation: "") }
			try? await Task.sleep(for: .milliseconds(30))
			let n = min(100_000, o.bytes.count - pos)
			_ = o.bytes[pos..<(pos + n)].withUnsafeBytes { sink.write($0, at: UInt64(pos), total: UInt64(o.bytes.count)) }
			pos += n
		}
		return DownloadReport(expectedSize: UInt64(o.bytes.count), bytesReceived: UInt64(pos))
	}
}

extension TempEnv {
	func request(_ cam: SlowCamera, handle: UInt32) -> TransferRequest {
		let info = cam.objects[handle]!.info
		return TransferRequest(identity: ObjectIdentity(cameraID: "cam1", info: info), lastKnownHandle: handle,
							   cameraDisplayName: "X-T30 III", format: info.format, dateCreated: info.dateCreated)
	}
}
