import Foundation

/// File formats the app can recognise. Detection uses the file's own bytes; the
/// PTP object format code and file extension are only hints.
public enum MediaFormat: String, Sendable, Codable, CaseIterable {
	case jpeg
	case heif
	case raf
	case mov
	case mp4
	case tiff
	case unknown

	public var displayName: String {
		switch self {
		case .jpeg: "JPEG"
		case .heif: "HEIF"
		case .raf: "RAF (RAW)"
		case .mov: "MOV 视频"
		case .mp4: "MP4 视频"
		case .tiff: "TIFF"
		case .unknown: "未知格式"
		}
	}

	public var isVideo: Bool { self == .mov || self == .mp4 }
	public var isStillImage: Bool { self == .jpeg || self == .heif || self == .raf || self == .tiff }

	/// Uniform Type Identifier string (kept as String so this file stays platform neutral).
	public var uniformTypeIdentifier: String? {
		switch self {
		case .jpeg: "public.jpeg"
		case .heif: "public.heic"
		case .raf: "com.fuji.raw-image"
		case .mov: "com.apple.quicktime-movie"
		case .mp4: "public.mpeg-4"
		case .tiff: "public.tiff"
		case .unknown: nil
		}
	}

	/// Best guess from the PTP ObjectFormat code and filename extension.
	public static func guess(objectFormat: UInt16, filename: String) -> MediaFormat {
		switch objectFormat {
		case 0x3801: return .jpeg
		case 0x300D: return .mov
		case 0xB982: return .mp4
		case 0x380D: return .tiff
		default: break
		}
		switch (filename as NSString).pathExtension.lowercased() {
		case "jpg", "jpeg": return .jpeg
		case "hif", "heif", "heic": return .heif
		case "raf": return .raf
		case "mov": return .mov
		case "mp4": return .mp4
		case "tif", "tiff": return .tiff
		default: return .unknown
		}
	}

	/// Detects the format from the first bytes of a file.
	public static func sniff(_ head: [UInt8]) -> MediaFormat {
		if head.count >= 3, head[0] == 0xFF, head[1] == 0xD8, head[2] == 0xFF { return .jpeg }
		if head.count >= 15, Array(head[0..<15]) == Array("FUJIFILMCCD-RAW".utf8) { return .raf }
		if head.count >= 4, (head[0] == 0x49 && head[1] == 0x49 && head[2] == 0x2A && head[3] == 0x00)
			|| (head[0] == 0x4D && head[1] == 0x4D && head[2] == 0x00 && head[3] == 0x2A) { return .tiff }
		if head.count >= 12, Array(head[4..<8]) == Array("ftyp".utf8) {
			let brand = String(decoding: head[8..<12], as: UTF8.self)
			switch brand {
			case "heic", "heix", "hevc", "heim", "heis", "mif1", "msf1": return .heif
			case "qt  ": return .mov
			default: return .mp4
			}
		}
		return .unknown
	}
}

/// Structural checks of a completed file. Detects truncated downloads that a
/// length check alone would miss (e.g. a camera reporting a reduced size).
public enum FileIntegrity {
	public enum Result: Sendable, Equatable {
		case ok
		case unverifiable(String)
		case corrupt(String)
	}

	public static func check(fileAt url: URL, format: MediaFormat, expectedSize: UInt64) -> Result {
		guard let attrs = try? FileManager.default.attributesOfItem(atPath: url.path),
			  let size = (attrs[.size] as? NSNumber)?.uint64Value else {
			return .corrupt("无法读取临时文件")
		}
		if size != expectedSize {
			return .corrupt("文件长度 \(size) 与相机报告的 \(expectedSize) 不一致")
		}
		guard let handle = try? FileHandle(forReadingFrom: url) else { return .corrupt("无法打开临时文件") }
		defer { try? handle.close() }
		let head = [UInt8]((try? handle.read(upToCount: 64)) ?? Data())
		let sniffed = MediaFormat.sniff(head)
		if format != .unknown, sniffed != .unknown, sniffed != format {
			// e.g. HEIF vs MOV brands; report but do not fail on brand ambiguity
			if !(format.isVideo && sniffed.isVideo) {
				return .corrupt("文件头为 \(sniffed.displayName)，与预期的 \(format.displayName) 不符")
			}
		}
		switch sniffed {
		case .jpeg:
			// A complete JPEG ends with the EOI marker (FF D9), possibly followed by padding.
			let tailLength = UInt64(min(4096, Int(size)))
			guard (try? handle.seek(toOffset: size - tailLength)) != nil,
				  let tail = try? handle.read(upToCount: Int(tailLength)) else {
				return .corrupt("无法读取文件尾")
			}
			let bytes = [UInt8](tail)
			var i = bytes.count - 1
			while i > 0, bytes[i] == 0x00 || bytes[i] == 0xFF { i -= 1 }
			if i >= 1, bytes[i - 1] == 0xFF, bytes[i] == 0xD9 { return .ok }
			if bytes.count >= 2, bytes[bytes.count - 2] == 0xFF, bytes[bytes.count - 1] == 0xD9 { return .ok }
			return .corrupt("JPEG 缺少结束标记，文件可能不完整")
		case .raf:
			// RAF header: offsets/lengths of the embedded JPEG and CFA data (big endian)
			guard head.count >= 64 else { return .corrupt("RAF 头部过短") }
			guard (try? handle.seek(toOffset: 0x54)) != nil, let hdr = try? handle.read(upToCount: 24), hdr.count == 24 else {
				return .unverifiable("无法读取 RAF 头")
			}
			let b = [UInt8](hdr)
			func be32(_ o: Int) -> UInt64 { UInt64(b[o]) << 24 | UInt64(b[o + 1]) << 16 | UInt64(b[o + 2]) << 8 | UInt64(b[o + 3]) }
			let jpegEnd = be32(0) + be32(4)
			let cfaEnd = be32(16) + be32(20)
			if jpegEnd > size || cfaEnd > size { return .corrupt("RAF 数据区超出文件长度，文件可能不完整") }
			return .ok
		case .heif, .mp4, .mov:
			return isoBoxesComplete(handle: handle, size: size)
		case .tiff:
			return .unverifiable("TIFF 仅校验长度")
		case .unknown:
			return .unverifiable("未知格式，仅校验长度")
		}
	}

	/// Walks top-level ISO BMFF boxes; the last box must end exactly at EOF.
	static func isoBoxesComplete(handle: FileHandle, size: UInt64) -> Result {
		var offset: UInt64 = 0
		var boxes = 0
		while offset < size {
			guard (try? handle.seek(toOffset: offset)) != nil, let h = try? handle.read(upToCount: 16), h.count >= 8 else {
				return .corrupt("ISO 媒体结构读取失败")
			}
			let b = [UInt8](h)
			var boxSize = UInt64(b[0]) << 24 | UInt64(b[1]) << 16 | UInt64(b[2]) << 8 | UInt64(b[3])
			if boxSize == 1 {
				guard b.count >= 16 else { return .corrupt("ISO 扩展长度缺失") }
				boxSize = (8..<16).reduce(UInt64(0)) { $0 << 8 | UInt64(b[$1]) }
			} else if boxSize == 0 {
				boxSize = size - offset
			}
			guard boxSize >= 8 else { return .corrupt("ISO 媒体结构无效") }
			offset += boxSize
			boxes += 1
			if boxes > 100_000 { return .unverifiable("ISO 结构过多") }
		}
		return offset == size ? .ok : .corrupt("ISO 媒体结构超出文件长度，文件可能不完整")
	}
}
