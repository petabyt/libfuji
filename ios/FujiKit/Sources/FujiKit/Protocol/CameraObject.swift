import Foundation
import CFujiCore

/// Object metadata as reported by the camera (PTP GetObjectInfo).
public struct CameraObjectInfo: Sendable, Hashable, Codable {
	public var storageID: UInt32
	public var objectFormat: UInt16
	public var protection: UInt16
	public var compressedSize: UInt32
	public var thumbFormat: UInt16
	public var thumbSize: UInt32
	public var thumbWidth: UInt32
	public var thumbHeight: UInt32
	public var imageWidth: UInt32
	public var imageHeight: UInt32
	public var imageBitDepth: UInt32
	public var parentObject: UInt32
	public var associationType: UInt16
	public var sequenceNumber: UInt32
	public var filename: String
	public var dateCreatedRaw: String
	public var dateModifiedRaw: String
	public var keywords: String

	public init(storageID: UInt32 = 0, objectFormat: UInt16, protection: UInt16 = 0, compressedSize: UInt32,
				thumbFormat: UInt16 = 0, thumbSize: UInt32 = 0, thumbWidth: UInt32 = 0, thumbHeight: UInt32 = 0,
				imageWidth: UInt32 = 0, imageHeight: UInt32 = 0, imageBitDepth: UInt32 = 0, parentObject: UInt32 = 0,
				associationType: UInt16 = 0, sequenceNumber: UInt32 = 0, filename: String,
				dateCreatedRaw: String = "", dateModifiedRaw: String = "", keywords: String = "") {
		self.storageID = storageID
		self.objectFormat = objectFormat
		self.protection = protection
		self.compressedSize = compressedSize
		self.thumbFormat = thumbFormat
		self.thumbSize = thumbSize
		self.thumbWidth = thumbWidth
		self.thumbHeight = thumbHeight
		self.imageWidth = imageWidth
		self.imageHeight = imageHeight
		self.imageBitDepth = imageBitDepth
		self.parentObject = parentObject
		self.associationType = associationType
		self.sequenceNumber = sequenceNumber
		self.filename = filename
		self.dateCreatedRaw = dateCreatedRaw
		self.dateModifiedRaw = dateModifiedRaw
		self.keywords = keywords
	}

	init(_ c: fujiios_object_info) {
		var c = c
		self.storageID = c.storage_id
		self.objectFormat = c.object_format
		self.protection = c.protection
		self.compressedSize = c.compressed_size
		self.thumbFormat = c.thumb_format
		self.thumbSize = c.thumb_compressed_size
		self.thumbWidth = c.thumb_width
		self.thumbHeight = c.thumb_height
		self.imageWidth = c.image_width
		self.imageHeight = c.image_height
		self.imageBitDepth = c.image_bit_depth
		self.parentObject = c.parent_object
		self.associationType = c.association_type
		self.sequenceNumber = c.sequence_number
		self.filename = withUnsafeBytes(of: &c.filename) { CString.string(from: $0) }
		self.dateCreatedRaw = withUnsafeBytes(of: &c.date_created) { CString.string(from: $0) }
		self.dateModifiedRaw = withUnsafeBytes(of: &c.date_modified) { CString.string(from: $0) }
		self.keywords = withUnsafeBytes(of: &c.keywords) { CString.string(from: $0) }
	}

	/// PTP association (folder) objects are not media.
	public var isFolder: Bool { objectFormat == 0x3001 }

	public var format: MediaFormat { MediaFormat.guess(objectFormat: objectFormat, filename: filename) }

	public var dateCreated: Date? { PTPDate.parse(dateCreatedRaw) }

	/// Camera-side orientation hint (libfuji maps "Orientation: 8" to 270°).
	public var orientationHint: String? {
		guard let r = keywords.range(of: "Orientation:") else { return nil }
		return keywords[r.upperBound...].trimmingCharacters(in: .whitespaces)
	}
}

enum CString {
	/// Decodes a fixed-size, NUL-terminated C char array.
	static func string(from raw: UnsafeRawBufferPointer) -> String {
		let bytes = raw.bindMemory(to: UInt8.self)
		let end = bytes.firstIndex(of: 0) ?? bytes.count
		return String(decoding: UnsafeBufferPointer(rebasing: bytes[0..<end]), as: UTF8.self)
	}
}

/// PTP DateTime strings: "YYYYMMDDThhmmss[.s][Z|+hhmm|-hhmm]". Cameras normally
/// send local time without a zone; such values are interpreted in `timeZone`
/// (the phone's zone by default) and flagged as `isLocalTime`.
public enum PTPDate {
	public static func parse(_ raw: String, timeZone: TimeZone = .current) -> Date? {
		parseDetailed(raw, timeZone: timeZone)?.date
	}

	public static func parseDetailed(_ raw: String, timeZone: TimeZone = .current) -> (date: Date, isLocalTime: Bool)? {
		let s = raw.trimmingCharacters(in: .whitespaces)
		let chars = Array(s)
		guard chars.count >= 15, chars[8] == "T" else { return nil }
		func num(_ a: Int, _ b: Int) -> Int? { Int(String(chars[a..<b])) }
		guard let year = num(0, 4), let month = num(4, 6), let day = num(6, 8),
			  let hour = num(9, 11), let minute = num(11, 13), let second = num(13, 15),
			  (1...12).contains(month), (1...31).contains(day), hour < 24, minute < 60, second < 61 else {
			return nil
		}
		var rest = chars.count > 15 ? String(chars[15...]) : ""
		var tenths = 0.0
		if rest.hasPrefix("."), rest.count >= 2, let d = Int(String(rest[rest.index(after: rest.startIndex)])) {
			tenths = Double(d) / 10
			rest = String(rest.dropFirst(2))
		}
		var zone = timeZone
		var isLocal = true
		if rest == "Z" {
			zone = TimeZone(identifier: "UTC")!
			isLocal = false
		} else if rest.count == 5, let sign = rest.first, sign == "+" || sign == "-",
				  let hh = Int(rest.dropFirst().prefix(2)), let mm = Int(rest.suffix(2)) {
			let seconds = (hh * 3600 + mm * 60) * (sign == "-" ? -1 : 1)
			zone = TimeZone(secondsFromGMT: seconds) ?? timeZone
			isLocal = false
		}
		var comps = DateComponents()
		comps.year = year; comps.month = month; comps.day = day
		comps.hour = hour; comps.minute = minute; comps.second = second
		var cal = Calendar(identifier: .gregorian)
		cal.timeZone = zone
		guard let date = cal.date(from: comps) else { return nil }
		return (date.addingTimeInterval(tenths), isLocal)
	}
}

/// Identity of a camera file that survives reconnects.
///
/// Over Wi-Fi libfuji enumerates objects as 1...N (ptp_fuji_get_object_handles),
/// so a handle only means "position in the current listing". Identity is
/// therefore camera + filename + size + capture time; the SHA-256 recorded
/// after download disambiguates recycled file names.
public struct ObjectIdentity: Hashable, Codable, Sendable, CustomStringConvertible {
	public var cameraID: String
	public var filename: String
	public var size: UInt64
	public var dateCreatedRaw: String

	public init(cameraID: String, filename: String, size: UInt64, dateCreatedRaw: String) {
		self.cameraID = cameraID
		self.filename = filename
		self.size = size
		self.dateCreatedRaw = dateCreatedRaw
	}

	public init(cameraID: String, info: CameraObjectInfo) {
		self.init(cameraID: cameraID, filename: info.filename, size: UInt64(info.compressedSize), dateCreatedRaw: info.dateCreatedRaw)
	}

	/// True when the camera supplied enough metadata to tell recycled names apart.
	public var isStrong: Bool { !dateCreatedRaw.isEmpty && size > 0 }

	public var key: String { "\(cameraID)|\(filename)|\(size)|\(dateCreatedRaw)" }

	public var description: String { "\(filename) (\(size) B, \(dateCreatedRaw.isEmpty ? "无日期" : dateCreatedRaw))" }
}
