import Foundation

/// PTP data type codes (ISO 15740 table 3).
public enum PTPDataType: UInt16, Sendable, Codable {
	case int8 = 0x0001, uint8 = 0x0002, int16 = 0x0003, uint16 = 0x0004
	case int32 = 0x0005, uint32 = 0x0006, int64 = 0x0007, uint64 = 0x0008
	case int128 = 0x0009, uint128 = 0x000A
	case aint8 = 0x4001, auint8 = 0x4002, aint16 = 0x4003, auint16 = 0x4004
	case aint32 = 0x4005, auint32 = 0x4006, aint64 = 0x4007, auint64 = 0x4008
	case aint128 = 0x4009, auint128 = 0x400A
	case string = 0xFFFF

	var scalarSize: Int? {
		switch self {
		case .int8, .uint8, .aint8, .auint8: 1
		case .int16, .uint16, .aint16, .auint16: 2
		case .int32, .uint32, .aint32, .auint32: 4
		case .int64, .uint64, .aint64, .auint64: 8
		case .int128, .uint128, .aint128, .auint128: 16
		case .string: nil
		}
	}

	var isArray: Bool { rawValue & 0x4000 != 0 && self != .string }
	var isSigned: Bool {
		switch self {
		case .int8, .int16, .int32, .int64, .int128, .aint8, .aint16, .aint32, .aint64, .aint128: true
		default: false
		}
	}
	var elementType: PTPDataType {
		isArray ? PTPDataType(rawValue: rawValue & ~0x4000)! : self
	}
}

public enum PTPValue: Hashable, Sendable, Codable, CustomStringConvertible {
	case int(Int64)
	case uint(UInt64)
	/// 128-bit values are kept as raw little-endian bytes.
	case wide([UInt8])
	case array([PTPValue])
	case string(String)

	public var integer: Int64? {
		switch self {
		case .int(let v): v
		case .uint(let v): v <= UInt64(Int64.max) ? Int64(v) : nil
		default: nil
		}
	}

	public var description: String {
		switch self {
		case .int(let v): "\(v)"
		case .uint(let v): "\(v)"
		case .wide(let b): "0x" + b.reversed().hexString
		case .array(let a): "[" + a.map(\.description).joined(separator: ", ") + "]"
		case .string(let s): "\"\(s)\""
		}
	}
}

public struct PropertyDescriptor: Sendable, Hashable {
	public enum Form: Sendable, Hashable {
		case none
		case range(min: PTPValue, max: PTPValue, step: PTPValue)
		case enumeration([PTPValue])
		/// MTP extended forms (DateTime, FixedLengthArray, RegEx, ByteArray, LongString)
		/// carry no list of allowed values; treated as not writable by the UI.
		case other(UInt8)
	}

	public var code: UInt16
	public var dataType: PTPDataType
	/// GetSet flag: false = read only.
	public var writable: Bool
	public var factoryDefault: PTPValue
	public var current: PTPValue
	public var form: Form

	/// The UI only offers editing when the camera says the property is writable
	/// AND lists the allowed values (enumeration or range). Writes of guessed
	/// values are never offered.
	public var isEditableWithKnownValues: Bool {
		guard writable else { return false }
		switch form {
		case .enumeration(let values): return !values.isEmpty
		// Ranges that cannot be listed (non-integer or more than 512 steps) stay read-only.
		case .range: return !allowedValues.isEmpty
		default: return false
		}
	}

	public var allowedValues: [PTPValue] {
		switch form {
		case .enumeration(let v):
			return v
		case .range(let lo, let hi, let step):
			guard let l = lo.integer, let h = hi.integer, let s = step.integer, s > 0, h >= l, (h - l) / s <= 512 else { return [] }
			return stride(from: l, through: h, by: Int(s)).map { dataType.isSigned ? .int($0) : .uint(UInt64($0)) }
		default:
			return []
		}
	}

	public static func parse(_ payload: [UInt8]) throws -> PropertyDescriptor {
		var r = ByteReader(payload)
		let code = try r.u16()
		let rawType = try r.u16()
		guard let type = PTPDataType(rawValue: rawType) else {
			throw ByteReader.Error.invalid(String(format: "unknown data type 0x%04X", rawType))
		}
		let getSet = try r.u8()
		let def = try readValue(&r, type)
		let cur = try readValue(&r, type)
		var form = Form.none
		if !r.isAtEnd {
			let flag = try r.u8()
			switch flag {
			case 0x00:
				form = .none
			case 0x01:
				form = .range(min: try readValue(&r, type), max: try readValue(&r, type), step: try readValue(&r, type))
			case 0x02:
				let count = Int(try r.u16())
				var values: [PTPValue] = []
				values.reserveCapacity(count)
				for _ in 0..<count { values.append(try readValue(&r, type)) }
				form = .enumeration(values)
			default:
				form = .other(flag)
			}
		}
		return PropertyDescriptor(code: code, dataType: type, writable: getSet == 1, factoryDefault: def, current: cur, form: form)
	}

	public static func readValue(_ r: inout ByteReader, _ type: PTPDataType) throws -> PTPValue {
		if type == .string { return .string(try r.ptpString()) }
		if type.isArray {
			let count = Int(try r.u32())
			guard count <= 65_536 else { throw ByteReader.Error.invalid("array too large") }
			var items: [PTPValue] = []
			for _ in 0..<count { items.append(try readValue(&r, type.elementType)) }
			return .array(items)
		}
		switch type {
		case .int8: return .int(Int64(Int8(bitPattern: try r.u8())))
		case .uint8: return .uint(UInt64(try r.u8()))
		case .int16: return .int(Int64(Int16(bitPattern: try r.u16())))
		case .uint16: return .uint(UInt64(try r.u16()))
		case .int32: return .int(Int64(Int32(bitPattern: try r.u32())))
		case .uint32: return .uint(UInt64(try r.u32()))
		case .int64: return .int(Int64(bitPattern: try r.u64()))
		case .uint64: return .uint(try r.u64())
		default: return .wide(try r.raw(16))
		}
	}

	/// Encodes a value for SetDevicePropValue using the descriptor's data type.
	public static func encode(_ value: PTPValue, as type: PTPDataType) throws -> [UInt8] {
		var w = ByteWriter()
		try encode(value, as: type, into: &w)
		return w.bytes
	}

	private static func encode(_ value: PTPValue, as type: PTPDataType, into w: inout ByteWriter) throws {
		if type == .string {
			guard case .string(let s) = value else { throw ByteReader.Error.invalid("expected string") }
			let units = Array(s.utf16)
			guard units.count < 255 else { throw ByteReader.Error.invalid("string too long") }
			if units.isEmpty { w.u8(0); return }
			w.u8(UInt8(units.count + 1))
			units.forEach { w.u16($0) }
			w.u16(0)
			return
		}
		if type.isArray {
			guard case .array(let items) = value else { throw ByteReader.Error.invalid("expected array") }
			w.u32(UInt32(items.count))
			for item in items { try encode(item, as: type.elementType, into: &w) }
			return
		}
		if case .wide(let bytes) = value {
			guard bytes.count == 16 else { throw ByteReader.Error.invalid("128-bit value must be 16 bytes") }
			w.raw(bytes)
			return
		}
		guard let v = value.integer else { throw ByteReader.Error.invalid("expected integer") }
		func check(_ lo: Int64, _ hi: Int64) throws {
			guard v >= lo && v <= hi else { throw ByteReader.Error.invalid("value \(v) out of range for \(type)") }
		}
		switch type {
		case .int8: try check(Int64(Int8.min), Int64(Int8.max)); w.u8(UInt8(bitPattern: Int8(v)))
		case .uint8: try check(0, Int64(UInt8.max)); w.u8(UInt8(v))
		case .int16: try check(Int64(Int16.min), Int64(Int16.max)); w.u16(UInt16(bitPattern: Int16(v)))
		case .uint16: try check(0, Int64(UInt16.max)); w.u16(UInt16(v))
		case .int32: try check(Int64(Int32.min), Int64(Int32.max)); w.i32(Int32(v))
		case .uint32: try check(0, Int64(UInt32.max)); w.u32(UInt32(v))
		case .int64: w.u64(UInt64(bitPattern: v))
		case .uint64: try check(0, Int64.max); w.u64(UInt64(v))
		default: throw ByteReader.Error.invalid("unsupported type \(type)")
		}
	}

	/// Decodes a raw GetDevicePropValue payload with a known type.
	public static func decodeValue(_ payload: [UInt8], as type: PTPDataType) throws -> PTPValue {
		var r = ByteReader(payload)
		return try readValue(&r, type)
	}
}
