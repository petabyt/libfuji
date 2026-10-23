import Foundation

/// One imported file. The SHA-256 of the received bytes is the definitive
/// duplicate key; the identity (camera, name, size, capture time) is used to
/// recognise a file before downloading it again.
public struct ImportRecord: Codable, Sendable, Hashable, Identifiable {
	public var id: UUID
	public var identity: ObjectIdentity
	public var sha256: String
	public var byteCount: UInt64
	public var format: MediaFormat
	/// Path relative to AppPaths.documents, nil if the local copy was removed.
	public var localRelativePath: String?
	public var photosLocalIdentifier: String?
	public var importedAt: Date
	/// True when the camera's "resize for smartphone" mode was on, i.e. the bytes
	/// are a camera-reduced JPEG and not the original file.
	public var isCameraReduced: Bool

	public init(id: UUID = UUID(), identity: ObjectIdentity, sha256: String, byteCount: UInt64, format: MediaFormat,
				localRelativePath: String?, photosLocalIdentifier: String?, importedAt: Date = Date(), isCameraReduced: Bool) {
		self.id = id
		self.identity = identity
		self.sha256 = sha256
		self.byteCount = byteCount
		self.format = format
		self.localRelativePath = localRelativePath
		self.photosLocalIdentifier = photosLocalIdentifier
		self.importedAt = importedAt
		self.isCameraReduced = isCameraReduced
	}
}

public enum DuplicateStatus: Sendable, Equatable {
	case new
	/// Same camera, name, size and capture time as an earlier import.
	case alreadyImported(ImportRecord)
	/// Same name and size but the camera gave no capture time: cannot tell
	/// whether it is the same photo or a recycled file name.
	case possibleDuplicate(ImportRecord)
}

public final class ImportStore: Sendable {
	private let state: Locked<[ImportRecord]>
	private let fileURL: URL

	public init(fileURL: URL) {
		self.fileURL = fileURL
		self.state = Locked(JSONFile.load([ImportRecord].self, from: fileURL) ?? [])
	}

	public var records: [ImportRecord] { state.value }

	public func status(for identity: ObjectIdentity) -> DuplicateStatus {
		let all = state.value
		if identity.isStrong, let r = all.first(where: { $0.identity == identity }) {
			return .alreadyImported(r)
		}
		if let r = all.first(where: {
			$0.identity.cameraID == identity.cameraID && $0.identity.filename == identity.filename && $0.identity.size == identity.size
		}) {
			return identity.isStrong && r.identity.isStrong ? .new : .possibleDuplicate(r)
		}
		return .new
	}

	public func record(sha256: String) -> ImportRecord? {
		state.value.first { $0.sha256 == sha256 }
	}

	public func add(_ record: ImportRecord) {
		state.withLock { $0.append(record) }
		persist()
	}

	public func update(_ id: UUID, _ change: @Sendable (inout ImportRecord) -> Void) {
		state.withLock { all in
			if let i = all.firstIndex(where: { $0.id == id }) { change(&all[i]) }
		}
		persist()
	}

	public func remove(_ id: UUID) {
		state.withLock { $0.removeAll { $0.id == id } }
		persist()
	}

	private func persist() {
		do {
			try JSONFile.save(state.value, to: fileURL)
		} catch {
			FujiLogger.shared.error("store", "保存导入记录失败：\(error.localizedDescription)")
		}
	}
}
