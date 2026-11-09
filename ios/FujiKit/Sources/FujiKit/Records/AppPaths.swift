import Foundation

/// File locations. Documents is exposed in the Files app (UIFileSharingEnabled),
/// so imported originals are reachable there without any re-encoding.
public struct AppPaths: Sendable {
	public let documents: URL
	public let applicationSupport: URL
	public let caches: URL

	public init(documents: URL, applicationSupport: URL, caches: URL) {
		self.documents = documents
		self.applicationSupport = applicationSupport
		self.caches = caches
	}

	public static func standard() -> AppPaths {
		let fm = FileManager.default
		let docs = fm.urls(for: .documentDirectory, in: .userDomainMask)[0]
		let support = fm.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0].appendingPathComponent("CamBridge", isDirectory: true)
		let caches = fm.urls(for: .cachesDirectory, in: .userDomainMask)[0].appendingPathComponent("CamBridge", isDirectory: true)
		return AppPaths(documents: docs, applicationSupport: support, caches: caches)
	}

	/// Final location of imported originals: Documents/导入/<相机>/<yyyy-MM-dd>/
	public var importsRoot: URL { documents.appendingPathComponent("导入", isDirectory: true) }
	/// In-progress downloads (.part files) live in Application Support so the
	/// system does not purge them like Caches, which keeps resumable state.
	public var partialDownloads: URL { applicationSupport.appendingPathComponent("Partial", isDirectory: true) }
	public var thumbnailCache: URL { caches.appendingPathComponent("Thumbnails", isDirectory: true) }
	public var pairingsFile: URL { applicationSupport.appendingPathComponent("pairings.json") }
	public var importRecordsFile: URL { applicationSupport.appendingPathComponent("imports.json") }
	public var transferQueueFile: URL { applicationSupport.appendingPathComponent("transfers.json") }
	public var settingsFile: URL { applicationSupport.appendingPathComponent("settings.json") }

	public func ensureDirectories() throws {
		for dir in [applicationSupport, caches, importsRoot, partialDownloads, thumbnailCache] {
			try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
		}
	}

	/// Bytes free for important data (Apple) or the filesystem free size (elsewhere).
	public func availableCapacity() -> Int64? {
		#if os(iOS) || os(macOS)
		if let values = try? documents.resourceValues(forKeys: [.volumeAvailableCapacityForImportantUsageKey]),
		   let cap = values.volumeAvailableCapacityForImportantUsage {
			return cap
		}
		#endif
		if let attrs = try? FileManager.default.attributesOfFileSystem(forPath: documents.path),
		   let free = (attrs[.systemFreeSize] as? NSNumber)?.int64Value {
			return free
		}
		return nil
	}
}

/// JSON helpers with atomic writes.
enum JSONFile {
	static func load<T: Decodable>(_ type: T.Type, from url: URL) -> T? {
		guard let data = try? Data(contentsOf: url) else { return nil }
		let decoder = JSONDecoder()
		decoder.dateDecodingStrategy = .iso8601
		return try? decoder.decode(type, from: data)
	}

	static func save<T: Encodable>(_ value: T, to url: URL) throws {
		let encoder = JSONEncoder()
		encoder.dateEncodingStrategy = .iso8601
		encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
		let data = try encoder.encode(value)
		try FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
		try data.write(to: url, options: [.atomic])
	}
}
