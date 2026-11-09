#if canImport(ImageIO)
import Foundation
import ImageIO

/// Reads display metadata from a downloaded original (or a thumbnail JPEG).
/// Values the file does not contain are reported as "未知", never guessed.
public enum ExifReader {
	public struct Item: Sendable, Hashable, Identifiable {
		public var id: String { label }
		public let label: String
		public let value: String
	}

	public static func items(fileAt url: URL) -> [Item] {
		guard let source = CGImageSourceCreateWithURL(url as CFURL, nil) else { return [] }
		return items(source)
	}

	public static func items(data: Data) -> [Item] {
		guard let source = CGImageSourceCreateWithData(data as CFData, nil) else { return [] }
		return items(source)
	}

	private static func items(_ source: CGImageSource) -> [Item] {
		guard let props = CGImageSourceCopyPropertiesAtIndex(source, 0, nil) as? [String: Any] else { return [] }
		let exif = props[kCGImagePropertyExifDictionary as String] as? [String: Any] ?? [:]
		let tiff = props[kCGImagePropertyTIFFDictionary as String] as? [String: Any] ?? [:]
		let gps = props[kCGImagePropertyGPSDictionary as String] as? [String: Any]
		var out: [Item] = []
		func add(_ label: String, _ value: String?) { out.append(Item(label: label, value: value ?? "未知")) }

		add("相机", [tiff[kCGImagePropertyTIFFMake as String] as? String, tiff[kCGImagePropertyTIFFModel as String] as? String]
			.compactMap { $0 }.joined(separator: " ").nilIfEmpty)
		add("镜头", exif[kCGImagePropertyExifLensModel as String] as? String)
		if let w = props[kCGImagePropertyPixelWidth as String] as? Int, let h = props[kCGImagePropertyPixelHeight as String] as? Int {
			add("像素", "\(w) × \(h)")
		} else {
			add("像素", nil)
		}
		add("拍摄时间", exif[kCGImagePropertyExifDateTimeOriginal as String] as? String)
		add("焦距", (exif[kCGImagePropertyExifFocalLength as String] as? Double).map { String(format: "%.0f mm", $0) })
		add("光圈", (exif[kCGImagePropertyExifFNumber as String] as? Double).map { String(format: "f/%.1f", $0) })
		add("快门", (exif[kCGImagePropertyExifExposureTime as String] as? Double).map { t in
			t >= 1 ? String(format: "%.1f 秒", t) : "1/\(Int((1 / t).rounded())) 秒"
		})
		add("ISO", (exif[kCGImagePropertyExifISOSpeedRatings as String] as? [Int])?.first.map { "\($0)" })
		add("曝光补偿", (exif[kCGImagePropertyExifExposureBiasValue as String] as? Double).map { String(format: "%+.1f EV", $0) })
		add("软件/固件", tiff[kCGImagePropertyTIFFSoftware as String] as? String)
		add("方向", (props[kCGImagePropertyOrientation as String] as? Int).map { "EXIF \($0)" })
		add("GPS", gps == nil ? "无" : "有")
		return out
	}
}
#endif
