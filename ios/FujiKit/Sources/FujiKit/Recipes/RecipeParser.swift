import Foundation
import CFujiCore

/// X RAW STUDIO recipe files (FP1/FP2/FP3) parsed with libfuji's fp library.
///
/// This is file parsing only. It does not read or change camera settings, does
/// not run the camera's RAW conversion, and is not a settings backup.
public struct Recipe: Sendable, Hashable {
	public struct Entry: Sendable, Hashable, Identifiable {
		public var id: String { key }
		public let key: String
		public let value: String
		public var label: String { Recipe.labels[key] ?? key }
	}

	public let entries: [Entry]
	/// False when fp could not name some values (see fp_dump_struct).
	public let complete: Bool

	public var device: String? { entries.first { $0.key == "Device" }?.value.nilIfEmpty }
	public var filmSimulation: String? { entries.first { $0.key == "FilmSimulation" }?.value }

	static let labels: [String: String] = [
		"Device": "适用机型",
		"DeviceVersion": "机型版本",
		"ProfileVersion": "配方格式",
		"IOPCode": "图像处理器代码",
		"StructVer": "结构版本",
		"TetherRAWConditonCode": "联机 RAW 条件码",
		"Editable": "可编辑",
		"ShootingCondition": "拍摄条件",
		"FileType": "输出类型",
		"ImageSize": "图像尺寸",
		"RotationAngle": "旋转角度",
		"ImageQuality": "画质",
		"ExposureBias": "曝光补偿",
		"DynamicRange": "动态范围",
		"WideDRange": "动态范围优先",
		"FilmSimulation": "胶片模拟",
		"BlackImageTone": "单色冷暖",
		"MonochromaticColor_RG": "单色品红/绿",
		"GrainEffect": "颗粒效果",
		"GrainEffectSize": "颗粒大小",
		"ChromeEffect": "色彩效果",
		"ColorChromeBlue": "蓝色彩效",
		"SmoothSkinEffect": "柔肤效果",
		"WBShootCond": "白平衡按拍摄条件",
		"WhiteBalance": "白平衡",
		"WBShiftR": "白平衡偏移 R",
		"WBShiftB": "白平衡偏移 B",
		"WBColorTemp": "色温",
		"HighlightTone": "高光",
		"ShadowTone": "阴影",
		"Color": "色彩",
		"Sharpness": "锐度",
		"NoisReduction": "降噪",
		"Clarity": "清晰度",
		"LensModulationOpt": "镜头调制优化",
		"ColorSpace": "色彩空间",
		"HDR": "HDR",
		"DigitalTeleConv": "数码增距",
		"PortraitEnhancer": "人像增强",
	]

	public enum ParseError: Error, LocalizedError {
		case failed(String)
		public var errorDescription: String? {
			switch self {
			case .failed(let why): "无法解析配方文件：\(why)"
			}
		}
	}

	/// Parses a local file. Copy security-scoped files into the app first.
	public static func parse(fileAt url: URL) throws -> Recipe {
		guard FileManager.default.isReadableFile(atPath: url.path) else {
			throw ParseError.failed("文件不可读")
		}
		// fp keeps a global error buffer; serialise calls.
		return try lock.withLock { _ in
			var buffer = [CChar](repeating: 0, count: 16 * 1024)
			let rc = fujiios_recipe_parse_file(url.path, &buffer, buffer.count)
			let text = String(decoding: buffer.prefix { $0 != 0 }.map { UInt8(bitPattern: $0) }, as: UTF8.self)
			let entries = parseLines(text)
			if rc != 0 && entries.count <= 3 {
				throw ParseError.failed(text.isEmpty ? "格式不受支持" : text)
			}
			return Recipe(entries: entries, complete: rc == 0)
		}
	}

	private static let lock = Locked(())

	static func parseLines(_ text: String) -> [Entry] {
		var out: [Entry] = []
		for line in text.split(separator: "\n") {
			let t = line.trimmingCharacters(in: .whitespaces)
			guard t.hasPrefix("<"), let close = t.firstIndex(of: ">") else { continue }
			let key = String(t[t.index(after: t.startIndex)..<close])
			guard let end = t.range(of: "</\(key)>") else { continue }
			let value = String(t[t.index(after: close)..<end.lowerBound])
			// fp writes IOPCode twice (hex and decimal); keep the first.
			if out.contains(where: { $0.key == key }) { continue }
			out.append(Entry(key: key, value: value))
		}
		return out
	}
}

extension String {
	var nilIfEmpty: String? { isEmpty ? nil : self }
}
