import Foundation
import Testing
@testable import FujiKit

@Suite struct SHA256Tests {
	@Test func nistVectors() {
		#expect(SHA256Hasher.hex(of: Data()) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
		#expect(SHA256Hasher.hex(of: Data("abc".utf8)) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
		#expect(SHA256Hasher.hex(of: Data("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq".utf8))
			== "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1")
	}

	@Test func millionAInChunks() {
		var h = SHA256Hasher()
		let chunk = Data(repeating: UInt8(ascii: "a"), count: 999)
		var remaining = 1_000_000
		while remaining > 0 {
			let n = min(remaining, chunk.count)
			h.update(chunk.prefix(n))
			remaining -= n
		}
		#expect(h.finalizeHex() == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0")
	}

	@Test func portableMatchesForRandomSplits() {
		var bytes = [UInt8](repeating: 0, count: 10_000)
		for i in bytes.indices { bytes[i] = UInt8(truncatingIfNeeded: i &* 31 &+ 7) }
		let whole = SHA256Hasher.hex(of: Data(bytes))
		var p = PortableSHA256()
		var i = 0
		for size in [1, 63, 64, 65, 127, 500, 4096] {
			let end = min(bytes.count, i + size)
			bytes[i..<end].withUnsafeBytes { p.update(bufferPointer: $0) }
			i = end
		}
		bytes[i...].withUnsafeBytes { p.update(bufferPointer: $0) }
		#expect(p.finalize().hexString == whole)
	}
}

@Suite struct PTPParsingTests {
	static func ptpString(_ s: String) -> [UInt8] {
		var w = ByteWriter()
		let units = Array(s.utf16)
		if units.isEmpty { w.u8(0); return w.bytes }
		w.u8(UInt8(units.count + 1))
		units.forEach { w.u16($0) }
		w.u16(0)
		return w.bytes
	}

	@Test func enumerationDescriptor() throws {
		var w = ByteWriter()
		w.u16(0x5005); w.u16(0x0004); w.u8(1) // code, uint16, get/set
		w.u16(2); w.u16(4)                    // default, current
		w.u8(2); w.u16(3); w.u16(2); w.u16(4); w.u16(0x8001) // enumeration
		let d = try PropertyDescriptor.parse(w.bytes)
		#expect(d.code == 0x5005)
		#expect(d.writable)
		#expect(d.current == .uint(4))
		#expect(d.allowedValues == [.uint(2), .uint(4), .uint(0x8001)])
		#expect(d.isEditableWithKnownValues)
		#expect(KnownProperty.named(0x5005)!.format(d.current, descriptor: d) == "日光")
	}

	@Test func noFormDescriptorDoesNotCrash() throws {
		// libpict's ptp_parse_prop_desc() calls ptp_panic() (abort) for this form.
		var w = ByteWriter()
		w.u16(0xD212); w.u16(0x0006); w.u8(0); w.u32(0); w.u32(7); w.u8(0)
		let d = try PropertyDescriptor.parse(w.bytes)
		#expect(d.form == .none)
		#expect(!d.isEditableWithKnownValues)
	}

	@Test func rangeAndSignedValues() throws {
		var w = ByteWriter()
		w.u16(0x5010); w.u16(0x0003); w.u8(1)
		w.u16(0); w.u16(UInt16(bitPattern: -667))
		w.u8(1); w.u16(UInt16(bitPattern: -3000)); w.u16(3000); w.u16(333)
		let d = try PropertyDescriptor.parse(w.bytes)
		#expect(d.current == .int(-667))
		#expect(d.allowedValues.first == .int(-3000))
		#expect(KnownProperty.named(0x5010)!.format(d.current, descriptor: d) == "-0.7 EV")
		#expect(try PropertyDescriptor.encode(.int(-667), as: .int16) == [0x65, 0xFD])
		#expect(throws: ByteReader.Error.self) { try PropertyDescriptor.encode(.int(70000), as: .int16) }
	}

	@Test func unlistableRangeIsNotEditable() throws {
		var w = ByteWriter()
		w.u16(0xD001); w.u16(0x0006); w.u8(1)
		w.u32(0); w.u32(5)
		w.u8(1); w.u32(0); w.u32(100_000); w.u32(1) // 100001 steps: too many to offer as choices
		let d = try PropertyDescriptor.parse(w.bytes)
		#expect(d.writable)
		#expect(d.allowedValues.isEmpty)
		#expect(!d.isEditableWithKnownValues)
	}

	@Test func stringDescriptorAndExtendedForm() throws {
		var w = ByteWriter()
		w.u16(0xD20B); w.u16(0xFFFF); w.u8(1)
		w.raw(Self.ptpString("")); w.raw(Self.ptpString("X-T30 III"))
		w.u8(0x03) // DateTime form (MTP)
		let d = try PropertyDescriptor.parse(w.bytes)
		#expect(d.current == .string("X-T30 III"))
		#expect(d.form == .other(3))
		#expect(!d.isEditableWithKnownValues)
		#expect(try PropertyDescriptor.encode(.string("AB"), as: .string) == [3, 0x41, 0, 0x42, 0, 0, 0])
	}

	@Test func truncatedDescriptorThrows() {
		#expect(throws: ByteReader.Error.self) { try PropertyDescriptor.parse([0x05, 0x50, 0x04]) }
		var w = ByteWriter()
		w.u16(0x5005); w.u16(0x0004); w.u8(1); w.u16(2); w.u16(4); w.u8(2); w.u16(1000) // count without values
		#expect(throws: ByteReader.Error.self) { try PropertyDescriptor.parse(w.bytes) }
	}

	@Test func batteryNeverInventsPercent() throws {
		var w = ByteWriter()
		w.u16(0x5001); w.u16(0x0002); w.u8(0); w.u8(0); w.u8(2)
		w.u8(1); w.u8(0); w.u8(3); w.u8(1) // range 0...3 step 1
		let d = try PropertyDescriptor.parse(w.bytes)
		let text = KnownProperty.named(0x5001)!.format(d.current, descriptor: d)
		#expect(text.contains("档位 2/3"))
		#expect(!text.contains("%"))
	}

	@Test func deviceInfo() throws {
		var w = ByteWriter()
		w.u16(100); w.u32(0x0E); w.u16(100); w.raw(Self.ptpString("fujifilm.co.jp: 1.0;")); w.u16(0)
		w.u32(2); w.u16(0x1001); w.u16(0x101B)
		w.u32(0)
		w.u32(1); w.u16(0xD212)
		w.u32(0); w.u32(1); w.u16(0x3801)
		w.raw(Self.ptpString("FUJIFILM")); w.raw(Self.ptpString("X-T30 III")); w.raw(Self.ptpString("1.01")); w.raw(Self.ptpString("ABC"))
		let info = try PTPDeviceInfo.parse(w.bytes)
		#expect(info.model == "X-T30 III")
		#expect(info.deviceVersion == "1.01")
		#expect(info.supports(operation: 0x101B))
		#expect(info.supports(property: 0xD212))
	}

	@Test func ptpDates() throws {
		let utc = TimeZone(identifier: "UTC")!
		let d = try #require(PTPDate.parseDetailed("20251123T104512", timeZone: utc))
		#expect(d.isLocalTime)
		#expect(d.date.timeIntervalSince1970 == 1763894712)
		let z = try #require(PTPDate.parseDetailed("20251123T104512.5+0800", timeZone: utc))
		#expect(!z.isLocalTime)
		#expect(z.date.timeIntervalSince1970 == 1763894712 - 8 * 3600 + 0.5)
		#expect(PTPDate.parse("") == nil)
		#expect(PTPDate.parse("2025-11-23") == nil)
		#expect(PTPDate.parse("20251399T999999") == nil)
	}

	@Test func objectIdentityStrength() {
		let strong = ObjectIdentity(cameraID: "c", filename: "DSCF0001.JPG", size: 10, dateCreatedRaw: "20250101T000000")
		let weak = ObjectIdentity(cameraID: "c", filename: "DSCF0001.JPG", size: 10, dateCreatedRaw: "")
		#expect(strong.isStrong)
		#expect(!weak.isStrong)
	}
}

@Suite struct BLEProtocolTests {
	@Test func legacyAdvertisement() throws {
		let m = Data([0xD8, 0x04, 0x02, 0xDE, 0xAD, 0xBE, 0xEF])
		let ad = try #require(FujiBLE.Advertisement.parse(manufacturerData: m, serviceUUIDs: [FujiBLE.Service.xapp.lowercased()], localName: "X-T30"))
		#expect(ad.kind == .legacyPairing(token: [0xDE, 0xAD, 0xBE, 0xEF]))
		#expect(ad.isPairable)
		#expect(ad.schemeHint == .legacyToken)
	}

	@Test func secureAdvertisement() throws {
		let m = Data([0xD8, 0x04, 0x01, 1, 2, 3, 4, 5])
		let pairing = try #require(FujiBLE.Advertisement.parse(manufacturerData: m, serviceUUIDs: [FujiBLE.Service.secureAdvertised], localName: nil))
		#expect(pairing.kind == .secure(serial: [1, 2, 3, 4, 5], pairingMode: true))
		let reconnect = try #require(FujiBLE.Advertisement.parse(manufacturerData: m, serviceUUIDs: [FujiBLE.Service.securePair], localName: nil))
		#expect(reconnect.kind == .secure(serial: [1, 2, 3, 4, 5], pairingMode: false))
		#expect(!reconnect.isPairable)
	}

	@Test func rejectsOtherCompanies() {
		#expect(FujiBLE.Advertisement.parse(manufacturerData: Data([0x4C, 0x00, 0x02, 1, 2, 3, 4]), serviceUUIDs: [], localName: nil) == nil)
		#expect(FujiBLE.Advertisement.parse(manufacturerData: nil, serviceUUIDs: [], localName: nil) == nil)
		let odd = FujiBLE.Advertisement.parse(manufacturerData: Data([0xD8, 0x04, 0x09]), serviceUUIDs: [], localName: nil)
		#expect(odd?.kind == .unrecognised(length: 3, type: 0x09))
	}

	@Test func shutterSequences() {
		#expect(FujiBLE.shutterWrites(.press) == [Data([1, 0]), Data([2, 0])])
		#expect(FujiBLE.shutterWrites(.focus) == [Data([1, 0]), Data([3, 0])])
		#expect(FujiBLE.shutterWrites(.release) == [Data([1, 0]), Data([0, 0])])
	}

	@Test func timeAndGeotagPackets() throws {
		let utc = TimeZone(identifier: "UTC")!
		let date = Date(timeIntervalSince1970: 1763894712) // 2025-11-23 10:45:12 UTC
		#expect(FujiBLE.timePacket(date, timeZone: utc) == Data([0xE9, 0x07, 11, 23, 10, 45, 12]))
		let geo = try #require(FujiBLE.geotagPacket(latitude: 31.2304, longitude: 121.4737, altitudeMeters: 12.4, timestamp: date))
		#expect(geo.count == 23)
		var r = ByteReader(geo)
		#expect(Int32(bitPattern: try r.u32()) == 312_304_000)
		#expect(Int32(bitPattern: try r.u32()) == 1_214_737_000)
		#expect(Int32(bitPattern: try r.u32()) == 12)
		#expect(try r.raw(4) == [0, 0, 0, 0])
		#expect(Data(try r.raw(7)) == Data([0xE9, 0x07, 11, 23, 10, 45, 12]))
		#expect(FujiBLE.geotagPacket(latitude: 123, longitude: 123, altitudeMeters: 0, timestamp: date) == nil)
	}

	@Test func secureAckAndStatus() {
		#expect(FujiBLE.secureStatusAck(Data([0x07, 0x96, 0x00, 0x00])) == Data([0x07, 0x96, 0x00, 0x20]))
		#expect(FujiBLE.secureStatusAck(Data([1, 2, 3])) == nil)
		#expect(FujiBLE.wifiReadiness(fromIndication1: Data([1, 0])) == .ready)
		#expect(FujiBLE.wifiReadiness(fromIndication1: Data([0])) == .busy)
		#expect(FujiBLE.wifiReadiness(fromIndication1: nil) == .unknown(nil))
		#expect(FujiBLE.isGeotagRequest(Data([1, 0])))
		#expect(!FujiBLE.isGeotagRequest(Data([2, 0])))
		#expect(FujiBLE.syncIntervalPacket(seconds: 10) == Data([10, 0]))
	}

	@Test func stringsAndNames() {
		#expect(FujiBLE.decodeString(Data("FUJIFILM-X-T30III-1234".utf8) + Data([0, 0])) == "FUJIFILM-X-T30III-1234")
		#expect(FujiBLE.decodeString(Data([0, 0])) == nil)
		#expect(FujiBLE.sanitizedClientName("张三的 iPhone 15 Pro Max Ultra Long") == "iPhone 15 Pro Max Ultra Lo")
		#expect(FujiBLE.sanitizedClientName("相机") == "iPhone")
	}
}

@Suite struct MiscTests {
	@Test func redactsSecrets() {
		let r = Redactor()
		r.register(secret: "hunter22")
		r.register(secretBytes: [0xDE, 0xAD, 0xBE, 0xEF])
		let out = r.redact("join ssid with hunter22, token deadbeef and password: abc123 ok")
		#expect(!out.contains("hunter22"))
		#expect(!out.contains("deadbeef"))
		#expect(!out.contains("abc123"))
		#expect(out.contains("ok"))
	}

	@Test func loggerDropsVerboseUnlessEnabled() {
		let log = FujiLogger(capacity: 10)
		log.verbose("t", "noise")
		log.info("t", "kept")
		#expect(log.entries().map(\.message) == ["kept"])
		log.verboseEnabled = true
		for i in 0..<20 { log.verbose("t", "\(i)") }
		#expect(log.entries().count == 10)
	}

	@Test func mediaSniffing() {
		#expect(MediaFormat.sniff([0xFF, 0xD8, 0xFF, 0xE1]) == .jpeg)
		#expect(MediaFormat.sniff(Array("FUJIFILMCCD-RAW 0201".utf8)) == .raf)
		#expect(MediaFormat.sniff([0, 0, 0, 0x18] + Array("ftypheic".utf8)) == .heif)
		#expect(MediaFormat.sniff([0, 0, 0, 0x14] + Array("ftypqt  ".utf8)) == .mov)
		#expect(MediaFormat.guess(objectFormat: 0x3000, filename: "DSCF0001.HIF") == .heif)
		#expect(MediaFormat.guess(objectFormat: 0x3801, filename: "x") == .jpeg)
	}

	@Test func fileIntegrity() throws {
		let dir = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
		try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
		defer { try? FileManager.default.removeItem(at: dir) }
		let good = dir.appendingPathComponent("a.jpg")
		try Data([0xFF, 0xD8, 0xFF, 0xE0] + [UInt8](repeating: 7, count: 100) + [0xFF, 0xD9]).write(to: good)
		#expect(FileIntegrity.check(fileAt: good, format: .jpeg, expectedSize: 106) == .ok)
		#expect(FileIntegrity.check(fileAt: good, format: .jpeg, expectedSize: 200) != .ok)
		let cut = dir.appendingPathComponent("b.jpg")
		try Data([0xFF, 0xD8, 0xFF, 0xE0] + [UInt8](repeating: 7, count: 100)).write(to: cut)
		if case .corrupt = FileIntegrity.check(fileAt: cut, format: .jpeg, expectedSize: 104) {} else { Issue.record("truncated JPEG accepted") }
		// ISO BMFF: ftyp box (16 bytes) + mdat box claiming 32 bytes but file ends early
		var iso = ByteWriter()
		iso.raw([0, 0, 0, 16]); iso.raw(Array("ftypheic".utf8)); iso.raw([0, 0, 0, 0])
		iso.raw([0, 0, 0, 32]); iso.raw(Array("mdat".utf8)); iso.raw([UInt8](repeating: 1, count: 8))
		let heif = dir.appendingPathComponent("c.hif")
		try iso.data.write(to: heif)
		if case .corrupt = FileIntegrity.check(fileAt: heif, format: .heif, expectedSize: 32) {} else { Issue.record("truncated HEIF accepted") }
	}

	@Test func recipeFixtures() throws {
		let fp1 = try #require(Bundle.module.url(forResource: "KodakPortra400", withExtension: "FP1", subdirectory: "Fixtures"))
		let r1 = try Recipe.parse(fileAt: fp1)
		#expect(r1.filmSimulation != nil)
		#expect(r1.entries.contains { $0.key == "WhiteBalance" })
		let fp2 = try #require(Bundle.module.url(forResource: "DSCF1810", withExtension: "FP2", subdirectory: "Fixtures"))
		let r2 = try Recipe.parse(fileAt: fp2)
		#expect(r2.device == "X-H1")
		let bad = FileManager.default.temporaryDirectory.appendingPathComponent("bad-\(UUID().uuidString).FP1")
		try Data("<not-xml".utf8).write(to: bad)
		defer { try? FileManager.default.removeItem(at: bad) }
		#expect(throws: Recipe.ParseError.self) { try Recipe.parse(fileAt: bad) }
	}

	@Test func workerRunsByPriority() async throws {
		let worker = PTPWorker(name: "test")
		let order = Locked<[String]>([])
		// Block the worker so the next three jobs queue up.
		let gate = Locked(false)
		let blocker = Task { try await worker.run(.normal) { while !gate.value { usleep(1000) }; order.withLock { $0.append("block") } } }
		try await Task.sleep(for: .milliseconds(50))
		let low = Task { try await worker.run(.background) { order.withLock { $0.append("low") } } }
		let normal = Task { try await worker.run(.normal) { order.withLock { $0.append("normal") } } }
		let high = Task { try await worker.run(.high) { order.withLock { $0.append("high") } } }
		try await Task.sleep(for: .milliseconds(50))
		gate.set(true)
		_ = try await (blocker.value, low.value, normal.value, high.value)
		#expect(order.value == ["block", "high", "normal", "low"])
		worker.stop()
	}

	@Test func workerCancelsQueuedJob() async throws {
		let worker = PTPWorker(name: "test-cancel")
		let gate = Locked(false)
		let ran = Locked(false)
		let blocker = Task { try await worker.run { while !gate.value { usleep(1000) } } }
		try await Task.sleep(for: .milliseconds(30))
		let queued = Task { try await worker.run { ran.set(true) } }
		try await Task.sleep(for: .milliseconds(30))
		queued.cancel()
		await #expect(throws: CancellationError.self) { try await queued.value }
		gate.set(true)
		try await blocker.value
		#expect(!ran.value)
		worker.stop()
	}
}
