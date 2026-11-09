import Foundation

/// Properties the app probes with GetDevicePropDesc. A property is shown only
/// if the camera returns a valid descriptor; it is editable only if the
/// descriptor says GetSet=1 and lists allowed values. Codes come from the
/// PTP standard (ISO 15740) or libfuji lib/fujiptp.h (whose vendor codes are
/// partly taken from libgphoto2 and marked there as possibly inaccurate).
public struct KnownProperty: Sendable, Identifiable, Hashable {
	public enum Source: String, Sendable {
		case ptpStandard = "PTP 标准"
		case fujiVendor = "富士扩展（fujiptp.h）"
	}

	public var id: UInt16 { code }
	public let code: UInt16
	public let name: String
	public let source: Source
	/// Value meaning is defined by the PTP standard; otherwise raw values are shown.
	public let hasStandardSemantics: Bool

	public static let all: [KnownProperty] = [
		KnownProperty(code: 0x5001, name: "电池电量", source: .ptpStandard, hasStandardSemantics: true),
		KnownProperty(code: 0x5005, name: "白平衡", source: .ptpStandard, hasStandardSemantics: true),
		KnownProperty(code: 0x5007, name: "光圈", source: .ptpStandard, hasStandardSemantics: true),
		KnownProperty(code: 0x500D, name: "快门速度", source: .ptpStandard, hasStandardSemantics: true),
		KnownProperty(code: 0x500E, name: "曝光程序", source: .ptpStandard, hasStandardSemantics: true),
		KnownProperty(code: 0x500F, name: "ISO 感光度", source: .ptpStandard, hasStandardSemantics: true),
		KnownProperty(code: 0x5010, name: "曝光补偿", source: .ptpStandard, hasStandardSemantics: true),
		KnownProperty(code: 0x500A, name: "对焦模式", source: .ptpStandard, hasStandardSemantics: false),
		KnownProperty(code: 0x500C, name: "闪光模式", source: .ptpStandard, hasStandardSemantics: false),
		KnownProperty(code: 0xD001, name: "胶片模拟", source: .fujiVendor, hasStandardSemantics: false),
		KnownProperty(code: 0xD007, name: "动态范围", source: .fujiVendor, hasStandardSemantics: false),
		KnownProperty(code: 0xD017, name: "色温", source: .fujiVendor, hasStandardSemantics: false),
		KnownProperty(code: 0xD018, name: "画质", source: .fujiVendor, hasStandardSemantics: false),
		KnownProperty(code: 0xD02A, name: "ISO（富士）", source: .fujiVendor, hasStandardSemantics: false),
		KnownProperty(code: 0xD36A, name: "电池信息 1", source: .fujiVendor, hasStandardSemantics: false),
		KnownProperty(code: 0xD36B, name: "电池信息 2", source: .fujiVendor, hasStandardSemantics: false),
		KnownProperty(code: 0xD153, name: "固件版本（属性）", source: .fujiVendor, hasStandardSemantics: false),
	]

	public static func named(_ code: UInt16) -> KnownProperty? { all.first { $0.code == code } }

	/// Human readable value. Unknown meanings are shown raw, never guessed.
	public func format(_ value: PTPValue, descriptor: PropertyDescriptor?) -> String {
		guard hasStandardSemantics, let v = value.integer else {
			return KnownProperty.raw(value)
		}
		switch code {
		case 0x5001:
			// Battery: report the position among the camera's allowed levels, not a fake percentage.
			if let d = descriptor {
				let levels = d.allowedValues.compactMap(\.integer)
				if let max = levels.max(), let min = levels.min(), max > min {
					return "档位 \(v - min)/\(max - min)（原始值 \(v)）"
				}
			}
			return "原始值 \(v)（无档位信息）"
		case 0x5005:
			let names: [Int64: String] = [1: "手动", 2: "自动", 3: "一键自动", 4: "日光", 5: "荧光灯", 6: "白炽灯", 7: "闪光灯"]
			return names[v] ?? String(format: "厂商值 0x%04llX", v)
		case 0x5007:
			return v > 0 ? String(format: "f/%.1f", Double(v) / 100) : "自动/未知"
		case 0x500D:
			guard v > 0, v != 0xFFFFFFFF else { return "自动/未知" }
			let seconds = Double(v) / 10000
			return seconds >= 1 ? String(format: "%.1f 秒", seconds) : "1/\(Int((1 / seconds).rounded())) 秒"
		case 0x500E:
			let names: [Int64: String] = [1: "手动 (M)", 2: "自动", 3: "光圈优先 (A)", 4: "快门优先 (S)", 5: "创意", 6: "动作", 7: "人像"]
			return names[v] ?? String(format: "厂商值 0x%04llX", v)
		case 0x500F:
			return v == 0xFFFF ? "自动" : "ISO \(v)"
		case 0x5010:
			return String(format: "%+.1f EV", Double(v) / 1000)
		default:
			return KnownProperty.raw(value)
		}
	}

	static func raw(_ value: PTPValue) -> String {
		if let i = value.integer { return String(format: "%lld (0x%llX)", i, i) }
		return value.description
	}
}
