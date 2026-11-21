import Foundation

/// What to download. Identity, not the handle, is authoritative: the handle is
/// only the position in the listing at the time the user picked the file.
public struct TransferRequest: Sendable, Codable, Hashable {
	public var identity: ObjectIdentity
	public var lastKnownHandle: UInt32
	public var cameraDisplayName: String
	public var format: MediaFormat
	public var dateCreated: Date?

	public init(identity: ObjectIdentity, lastKnownHandle: UInt32, cameraDisplayName: String, format: MediaFormat, dateCreated: Date?) {
		self.identity = identity
		self.lastKnownHandle = lastKnownHandle
		self.cameraDisplayName = cameraDisplayName
		self.format = format
		self.dateCreated = dateCreated
	}
}

public enum TransferFailure: Error, LocalizedError, Sendable, Equatable {
	case objectNotFound
	case objectChanged
	case insufficientSpace(needed: Int64, available: Int64)
	case fileTooLarge(UInt64)
	case writeFailed(String)
	case integrity(String)
	case ptp(PTPError)
	case cancelled

	public var errorDescription: String? {
		switch self {
		case .objectNotFound: "相机上找不到该文件（可能已删除、存储卡已更换或相机切换了显示范围）"
		case .objectChanged: "下载前相机文件列表发生变化，已停止以免下载错误的文件"
		case .insufficientSpace(let needed, let available):
			"iPhone 存储空间不足：需要约 \(ByteCountFormatter.string(fromByteCount: needed, countStyle: .file))，可用 \(ByteCountFormatter.string(fromByteCount: available, countStyle: .file))"
		case .fileTooLarge(let size): "文件大小 \(size) 字节超过 PTP GetPartialObject 32 位偏移的上限（4 GB）"
		case .writeFailed(let why): "写入临时文件失败：\(why)"
		case .integrity(let why): "文件校验失败：\(why)"
		case .ptp(let e): e.localizedDescription
		case .cancelled: "已取消"
		}
	}

	/// Worth retrying after reconnecting.
	public var isRetryable: Bool {
		switch self {
		case .ptp(let e): return e.isConnectionLost
		case .writeFailed: return true
		default: return false
		}
	}
}

public struct TransferOptions: Sendable {
	/// Read D226/D227 before and after the download (diagnostics).
	public var auditProperties = false
	/// The camera's "resize for smartphone" mode was active for this download.
	public var cameraReduced = false
	/// Size of the region re-read from the camera to verify a resume point.
	public var resumeVerifyWindow: UInt32 = 64 * 1024
	/// Extra free space required on top of the file size.
	public var spaceMargin: Int64 = 32 * 1024 * 1024
	public init() {}
}

public struct TransferOutcome: Sendable {
	public enum Kind: Sendable {
		case imported(ImportRecord)
		/// Bytes identical to an earlier import (same SHA-256); nothing new saved.
		case duplicate(ImportRecord)
	}
	public var kind: Kind
	public var fileURL: URL?
	public var report: DownloadReport?
	public var resumedFrom: UInt64
	public var integrity: FileIntegrity.Result
	public var notes: [String]
	public var bytesTransferred: UInt64
	public var duration: TimeInterval
}

/// Downloads one camera file into the Documents import folder.
public struct TransferEngine: Sendable {
	public typealias ProgressHandler = @Sendable (_ received: UInt64, _ total: UInt64) -> Void

	public let paths: AppPaths
	public let store: ImportStore
	public let logger: FujiLogger

	public init(paths: AppPaths, store: ImportStore, logger: FujiLogger) {
		self.paths = paths
		self.store = store
		self.logger = logger
	}

	static func matches(_ info: CameraObjectInfo, _ identity: ObjectIdentity) -> Bool {
		guard info.filename == identity.filename, UInt64(info.compressedSize) == identity.size else { return false }
		if info.dateCreatedRaw.isEmpty || identity.dateCreatedRaw.isEmpty { return true }
		return info.dateCreatedRaw == identity.dateCreatedRaw
	}

	/// Finds the current handle of a file after reconnecting or a listing change.
	@concurrent
	public func resolveHandle(_ request: TransferRequest, source: any CameraObjectSource) async throws -> (UInt32, CameraObjectInfo) {
		do {
			let info = try await source.objectInfo(handle: request.lastKnownHandle)
			if Self.matches(info, request.identity) { return (request.lastKnownHandle, info) }
		} catch let e as PTPError where e.isConnectionLost {
			throw TransferFailure.ptp(e)
		} catch {
			// handle no longer valid: fall through to a rescan
		}
		let handles: [UInt32]
		do { handles = try await source.objectHandles() } catch let e as PTPError { throw TransferFailure.ptp(e) }
		// Look near the old position first: new shots shift indices by a few.
		let ordered = handles.sorted {
			abs(Int64($0) - Int64(request.lastKnownHandle)) < abs(Int64($1) - Int64(request.lastKnownHandle))
		}
		for h in ordered where h != request.lastKnownHandle {
			try Task.checkCancellation()
			do {
				let info = try await source.objectInfo(handle: h)
				if Self.matches(info, request.identity) {
					logger.info("transfer", "\(request.identity.filename) 的句柄由 \(request.lastKnownHandle) 变为 \(h)")
					return (h, info)
				}
			} catch let e as PTPError where e.isConnectionLost {
				throw TransferFailure.ptp(e)
			} catch {
				continue
			}
		}
		throw TransferFailure.objectNotFound
	}

	private func partialURLs(_ identity: ObjectIdentity) -> (data: URL, meta: URL) {
		let name = SHA256Hasher.hex(of: Data(identity.key.utf8)).prefix(32)
		return (paths.partialDownloads.appendingPathComponent("\(name).part"),
				paths.partialDownloads.appendingPathComponent("\(name).json"))
	}

	private struct PartialMeta: Codable { var identity: ObjectIdentity; var expectedSize: UInt64 }

	/// Removes the partial file for a request (used when the user cancels).
	public func discardPartial(_ identity: ObjectIdentity) {
		let urls = partialURLs(identity)
		try? FileManager.default.removeItem(at: urls.data)
		try? FileManager.default.removeItem(at: urls.meta)
	}

	@concurrent
	public func run(_ request: TransferRequest, source: any CameraObjectSource, options: TransferOptions,
					progress: @escaping ProgressHandler) async throws -> TransferOutcome {
		let started = Date()
		var notes: [String] = []
		try paths.ensureDirectories()

		let (handle, info) = try await resolveHandle(request, source: source)
		let expected = UInt64(info.compressedSize)
		guard expected > 0 else { throw TransferFailure.integrity("相机报告的文件大小为 0") }
		guard expected < UInt64(UInt32.max) else { throw TransferFailure.fileTooLarge(expected) }

		// Resume only after proving the camera returns the same bytes at the resume point.
		let urls = partialURLs(request.identity)
		let fm = FileManager.default
		var startOffset: UInt64 = 0
		if let meta = JSONFile.load(PartialMeta.self, from: urls.meta), meta.identity == request.identity, meta.expectedSize == expected,
		   let attrs = try? fm.attributesOfItem(atPath: urls.data.path), let have = (attrs[.size] as? NSNumber)?.uint64Value,
		   have > 0, have <= expected {
			let window = UInt64(min(UInt64(options.resumeVerifyWindow), have))
			let windowStart = have - window
			do {
				let remote = try await source.readPartial(handle: handle, offset: UInt32(windowStart), length: UInt32(window))
				let local = try Self.readLocal(urls.data, offset: windowStart, length: Int(window))
				if remote == local {
					startOffset = have
					notes.append("已用相机偏移读取校验末尾 \(window) 字节，从 \(have) 字节处续传")
				} else {
					notes.append("续传校验不一致（相机数据与本地已下载部分不同），改为重新下载")
				}
			} catch let e as PTPError where e.isConnectionLost {
				throw TransferFailure.ptp(e)
			} catch {
				notes.append("无法校验续传点（\(error.localizedDescription)），改为重新下载")
			}
		}
		if startOffset == 0 {
			try? fm.removeItem(at: urls.data)
		}
		try JSONFile.save(PartialMeta(identity: request.identity, expectedSize: expected), to: urls.meta)
		if !fm.fileExists(atPath: urls.data.path) {
			guard fm.createFile(atPath: urls.data.path, contents: nil) else {
				throw TransferFailure.writeFailed("无法创建临时文件")
			}
		}

		// Space check before touching the camera.
		let needed = Int64(expected - startOffset) + options.spaceMargin
		if let available = paths.availableCapacity(), available < needed {
			throw TransferFailure.insufficientSpace(needed: needed, available: available)
		}

		var report: DownloadReport?
		if startOffset < expected {
			let sink = try FileSink(url: urls.data, identity: request.identity, startOffset: startOffset, progress: progress)
			do {
				report = try await source.download(handle: handle, from: startOffset, audit: options.auditProperties, sink: sink)
				try sink.finish()
			} catch let e as PTPError {
				try? sink.finish()
				if let sinkError = sink.failure { throw sinkError }
				throw e.isCancelled ? TransferFailure.cancelled : TransferFailure.ptp(e)
			} catch is CancellationError {
				try? sink.finish()
				throw TransferFailure.cancelled
			}
			if let r = report {
				logger.info("transfer", "\(request.identity.filename)：\(r.auditSummary)")
				if r.bytesReceived != expected {
					throw TransferFailure.integrity("收到 \(r.bytesReceived) 字节，相机报告 \(expected) 字节")
				}
			}
		} else {
			progress(expected, expected)
		}

		let integrity = FileIntegrity.check(fileAt: urls.data, format: request.format == .unknown ? info.format : request.format, expectedSize: expected)
		if case .corrupt(let why) = integrity {
			discardPartial(request.identity)
			throw TransferFailure.integrity(why)
		}
		if case .unverifiable(let why) = integrity { notes.append(why) }

		let sha = try SHA256Hasher.hex(ofFileAt: urls.data)
		if let existing = store.record(sha256: sha) {
			discardPartial(request.identity)
			return TransferOutcome(kind: .duplicate(existing), fileURL: existing.localRelativePath.map { paths.documents.appendingPathComponent($0) },
								   report: report, resumedFrom: startOffset, integrity: integrity, notes: notes + ["内容与已导入文件完全相同（SHA-256），未重复保存"],
								   bytesTransferred: expected - startOffset, duration: Date().timeIntervalSince(started))
		}

		let destination = try uniqueDestination(for: request, sha256: sha)
		try fm.createDirectory(at: destination.deletingLastPathComponent(), withIntermediateDirectories: true)
		try fm.moveItem(at: urls.data, to: destination)
		try? fm.removeItem(at: urls.meta)

		let relative = String(destination.path.dropFirst(paths.documents.path.count)).trimmingCharacters(in: CharacterSet(charactersIn: "/"))
		let record = ImportRecord(identity: request.identity, sha256: sha, byteCount: expected, format: info.format,
								  localRelativePath: relative, photosLocalIdentifier: nil, isCameraReduced: options.cameraReduced)
		store.add(record)
		return TransferOutcome(kind: .imported(record), fileURL: destination, report: report, resumedFrom: startOffset,
							   integrity: integrity, notes: notes, bytesTransferred: expected - startOffset,
							   duration: Date().timeIntervalSince(started))
	}

	static func readLocal(_ url: URL, offset: UInt64, length: Int) throws -> [UInt8] {
		let h = try FileHandle(forReadingFrom: url)
		defer { try? h.close() }
		try h.seek(toOffset: offset)
		return [UInt8](try h.read(upToCount: length) ?? Data())
	}

	private func uniqueDestination(for request: TransferRequest, sha256: String) throws -> URL {
		let camera = Self.sanitize(request.cameraDisplayName.isEmpty ? "相机" : request.cameraDisplayName)
		let day: String = {
			let f = DateFormatter()
			f.locale = Locale(identifier: "en_US_POSIX")
			f.dateFormat = "yyyy-MM-dd"
			return f.string(from: request.dateCreated ?? Date())
		}()
		let folder = paths.importsRoot.appendingPathComponent(camera, isDirectory: true).appendingPathComponent(day, isDirectory: true)
		let base = Self.sanitize(request.identity.filename.isEmpty ? "未命名" : request.identity.filename)
		var candidate = folder.appendingPathComponent(base)
		var n = 1
		let stem = (base as NSString).deletingPathExtension
		let ext = (base as NSString).pathExtension
		while FileManager.default.fileExists(atPath: candidate.path) {
			// Same name, different content (SHA differs, else we'd have returned
			// .duplicate): keep both, e.g. DSCF0001-1.JPG after a counter reset.
			candidate = folder.appendingPathComponent(ext.isEmpty ? "\(stem)-\(n)" : "\(stem)-\(n).\(ext)")
			n += 1
		}
		return candidate
	}

	static func sanitize(_ name: String) -> String {
		let bad = CharacterSet(charactersIn: "/\\:?%*|\"<>\0")
		let cleaned = name.unicodeScalars.map { bad.contains($0) ? "_" : String($0) }.joined()
		return cleaned.isEmpty ? "_" : cleaned
	}
}

/// Writes download chunks to the .part file on the PTP worker thread.
final class FileSink: DownloadSink, @unchecked Sendable {
	private let handle: FileHandle
	private let identity: ObjectIdentity
	private let progress: TransferEngine.ProgressHandler
	private var written: UInt64
	private(set) var failure: TransferFailure?
	private var closed = false

	init(url: URL, identity: ObjectIdentity, startOffset: UInt64, progress: @escaping TransferEngine.ProgressHandler) throws {
		do {
			handle = try FileHandle(forWritingTo: url)
			try handle.truncate(atOffset: startOffset)
			try handle.seek(toOffset: startOffset)
		} catch {
			throw TransferFailure.writeFailed(error.localizedDescription)
		}
		self.identity = identity
		self.written = startOffset
		self.progress = progress
	}

	func begin(info: CameraObjectInfo, startOffset: UInt64) -> Bool {
		// The listing may have changed between resolving and downloading.
		guard TransferEngine.matches(info, identity) else {
			failure = .objectChanged
			return false
		}
		progress(written, UInt64(info.compressedSize))
		return true
	}

	func write(_ bytes: UnsafeRawBufferPointer, at offset: UInt64, total: UInt64) -> Bool {
		guard offset == written else {
			failure = .writeFailed("数据块顺序错误（期望偏移 \(written)，收到 \(offset)）")
			return false
		}
		do {
			try handle.write(contentsOf: Data(bytes))
		} catch {
			let ns = error as NSError
			if ns.code == 28 /* ENOSPC */ || ns.code == 640 /* NSFileWriteOutOfSpaceError */ {
				failure = .insufficientSpace(needed: Int64(total - written), available: 0)
			} else {
				failure = .writeFailed(error.localizedDescription)
			}
			return false
		}
		written += UInt64(bytes.count)
		progress(written, total)
		return true
	}

	func finish() throws {
		guard !closed else { return }
		closed = true
		do {
			try handle.synchronize()
			try handle.close()
		} catch {
			throw TransferFailure.writeFailed(error.localizedDescription)
		}
	}
}
