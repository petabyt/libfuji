import Foundation

/// Bounds-checked little-endian reader for PTP payloads. Every read throws
/// instead of reading past the end, so malformed camera data cannot crash the app.
public struct ByteReader: Sendable {
	public enum Error: Swift.Error, Sendable, Equatable {
		case truncated(offset: Int, needed: Int, available: Int)
		case invalid(String)
	}

	public let bytes: [UInt8]
	public private(set) var offset: Int = 0

	public init(_ bytes: [UInt8]) { self.bytes = bytes }
	public init(_ data: Data) { self.bytes = [UInt8](data) }

	public var remaining: Int { bytes.count - offset }
	public var isAtEnd: Bool { offset >= bytes.count }

	private mutating func take(_ n: Int) throws -> ArraySlice<UInt8> {
		guard n >= 0, remaining >= n else {
			throw Error.truncated(offset: offset, needed: n, available: remaining)
		}
		defer { offset += n }
		return bytes[offset..<(offset + n)]
	}

	public mutating func u8() throws -> UInt8 { try take(1).first! }

	public mutating func u16() throws -> UInt16 {
		let s = try take(2)
		return UInt16(s[s.startIndex]) | UInt16(s[s.startIndex + 1]) << 8
	}

	public mutating func u32() throws -> UInt32 {
		let s = try take(4)
		var v: UInt32 = 0
		for (i, b) in s.enumerated() { v |= UInt32(b) << (8 * UInt32(i)) }
		return v
	}

	public mutating func u64() throws -> UInt64 {
		let s = try take(8)
		var v: UInt64 = 0
		for (i, b) in s.enumerated() { v |= UInt64(b) << (8 * UInt64(i)) }
		return v
	}

	public mutating func skip(_ n: Int) throws { _ = try take(n) }

	public mutating func raw(_ n: Int) throws -> [UInt8] { Array(try take(n)) }

	/// PTP string: u8 count of UTF-16 code units (including the terminator), then UTF-16LE.
	public mutating func ptpString() throws -> String {
		let count = Int(try u8())
		guard count > 0 else { return "" }
		var units: [UInt16] = []
		units.reserveCapacity(count)
		for _ in 0..<count { units.append(try u16()) }
		while let last = units.last, last == 0 { units.removeLast() }
		return String(decoding: units, as: UTF16.self)
	}

	/// PTP array of u16: u32 count then elements. `limit` guards against absurd counts.
	public mutating func u16Array(limit: Int = 4096) throws -> [UInt16] {
		let count = Int(try u32())
		guard count <= limit else { throw Error.invalid("array count \(count) exceeds \(limit)") }
		guard remaining >= count * 2 else { throw Error.truncated(offset: offset, needed: count * 2, available: remaining) }
		return try (0..<count).map { _ in try u16() }
	}

	public mutating func u32Array(limit: Int = 1 << 20) throws -> [UInt32] {
		let count = Int(try u32())
		guard count <= limit else { throw Error.invalid("array count \(count) exceeds \(limit)") }
		guard remaining >= count * 4 else { throw Error.truncated(offset: offset, needed: count * 4, available: remaining) }
		return try (0..<count).map { _ in try u32() }
	}
}

/// Little-endian writer used for BLE packets and PTP property values.
public struct ByteWriter: Sendable {
	public private(set) var bytes: [UInt8] = []

	public init() {}

	public mutating func u8(_ v: UInt8) { bytes.append(v) }
	public mutating func u16(_ v: UInt16) { bytes.append(UInt8(v & 0xff)); bytes.append(UInt8(v >> 8)) }
	public mutating func u32(_ v: UInt32) { for i in 0..<4 { bytes.append(UInt8((v >> (8 * UInt32(i))) & 0xff)) } }
	public mutating func i32(_ v: Int32) { u32(UInt32(bitPattern: v)) }
	public mutating func u64(_ v: UInt64) { for i in 0..<8 { bytes.append(UInt8((v >> (8 * UInt64(i))) & 0xff)) } }
	public mutating func raw(_ b: [UInt8]) { bytes.append(contentsOf: b) }

	public var data: Data { Data(bytes) }
}

extension Array where Element == UInt8 {
	public var hexString: String { map { String(format: "%02x", $0) }.joined() }
}

extension Data {
	public var hexString: String { [UInt8](self).hexString }
}
