import Foundation
import Observation

/// Saves an imported file into the system photo library (implemented with
/// PhotoKit in the app). Returns the asset's local identifier.
public protocol MediaLibrarySaver: Sendable {
	func save(fileAt url: URL, format: MediaFormat, originalFilename: String, creationDate: Date?) async throws -> String
}

public struct TransferItem: Identifiable, Codable, Sendable, Hashable {
	public enum State: String, Codable, Sendable {
		case queued
		case running
		case saving
		case completed
		case duplicate
		case failed
		case cancelled
		/// Interrupted by a lost connection or app suspension; resumes after reconnecting.
		case waitingForConnection

		public var label: String {
			switch self {
			case .queued: "排队中"
			case .running: "下载中"
			case .saving: "保存中"
			case .completed: "已完成"
			case .duplicate: "重复，已跳过"
			case .failed: "失败"
			case .cancelled: "已取消"
			case .waitingForConnection: "等待重新连接"
			}
		}

		public var isFinished: Bool { self == .completed || self == .duplicate || self == .failed || self == .cancelled }
	}

	public enum PhotosStatus: Codable, Sendable, Hashable {
		case notRequested
		case saved(String)
		case denied
		case failed(String)
	}

	public var id: UUID
	public var request: TransferRequest
	public var state: State
	public var bytesReceived: UInt64
	public var totalBytes: UInt64
	public var attempts: Int
	public var lastError: String?
	public var notes: [String]
	public var localRelativePath: String?
	public var photosStatus: PhotosStatus
	public var cameraReduced: Bool
	/// Download even if the import records say it was already imported.
	public var force: Bool
	public var addedAt: Date
	public var finishedAt: Date?
	public var averageSpeed: Double?

	public init(request: TransferRequest, cameraReduced: Bool, force: Bool = false) {
		self.id = UUID()
		self.request = request
		self.state = .queued
		self.bytesReceived = 0
		self.totalBytes = request.identity.size
		self.attempts = 0
		self.notes = []
		self.photosStatus = .notRequested
		self.cameraReduced = cameraReduced
		self.force = force
		self.addedAt = Date()
	}

	public var fractionCompleted: Double {
		totalBytes > 0 ? min(1, Double(bytesReceived) / Double(totalBytes)) : 0
	}
}

/// Ordered download queue. One file at a time: the camera session is a single
/// sequential PTP command stream, and the worker also serves thumbnails and
/// events between files.
@MainActor
@Observable
public final class TransferQueue {
	public private(set) var items: [TransferItem] = []
	/// Bytes per second, smoothed.
	public private(set) var currentSpeed: Double = 0
	public private(set) var isProcessing = false
	public var isPaused = false { didSet { if !isPaused { pump() } } }
	public var saveToPhotos = true
	public var maxAttempts = 3

	@ObservationIgnored public var optionsProvider: () -> TransferOptions = { TransferOptions() }
	@ObservationIgnored public var saver: (any MediaLibrarySaver)?
	/// Called when an item finishes or the queue drains (used for background task progress).
	@ObservationIgnored public var onChange: (() -> Void)?
	/// Called right before an item downloads (the app sets session-wide camera
	/// flags such as libfuji's compress_images_for_smartphone to match the item).
	@ObservationIgnored public var willStart: ((TransferItem) async -> Void)?

	@ObservationIgnored private var source: (any CameraObjectSource)?
	@ObservationIgnored private let engine: TransferEngine
	@ObservationIgnored private let fileURL: URL
	@ObservationIgnored private var runner: Task<Void, Never>?
	@ObservationIgnored private var currentTask: Task<TransferOutcome, Error>?
	@ObservationIgnored private var currentID: UUID?
	@ObservationIgnored private var userCancelled: Set<UUID> = []
	@ObservationIgnored private let progressBox = Locked<(UInt64, UInt64)>((0, 0))
	@ObservationIgnored private var sampler: Task<Void, Never>?

	public init(engine: TransferEngine, fileURL: URL) {
		self.engine = engine
		self.fileURL = fileURL
		if let stored = JSONFile.load([TransferItem].self, from: fileURL) {
			// Anything that was in flight when the app stopped resumes after reconnecting.
			items = stored.map { item in
				var i = item
				if i.state == .running || i.state == .saving || i.state == .queued {
					i.state = .waitingForConnection
					i.notes.append("应用上次退出时传输被中断")
				}
				return i
			}
		}
	}

	// MARK: source attachment

	/// Called after a camera session is (re)established.
	public func attach(source: any CameraObjectSource) {
		self.source = source
		for i in items.indices where items[i].state == .waitingForConnection {
			items[i].state = .queued
		}
		save()
		pump()
	}

	/// Called when the session is lost or closed. The running download is
	/// interrupted; its partial file is kept for a verified resume.
	public func detach(reason: String) {
		source = nil
		currentTask?.cancel()
		for i in items.indices where items[i].state == .queued || items[i].state == .running {
			items[i].state = .waitingForConnection
			items[i].lastError = reason
		}
		save()
	}

	public var hasSource: Bool { source != nil }

	/// Stops after interrupting the running file (partial data is kept), e.g.
	/// when a background task expires. `resume()` continues.
	public func suspend(reason: String) {
		isPaused = true
		if let id = currentID, let i = items.firstIndex(where: { $0.id == id }) {
			items[i].lastError = reason
		}
		currentTask?.cancel()
	}

	public func resume() {
		for i in items.indices where items[i].state == .waitingForConnection && source != nil {
			items[i].state = .queued
		}
		isPaused = false
		save()
		pump()
	}

	// MARK: queue editing

	@discardableResult
	public func enqueue(_ requests: [TransferRequest], cameraReduced: Bool, force: Bool = false) -> Int {
		var added = 0
		for r in requests {
			let active = items.contains { $0.request.identity == r.identity && !$0.state.isFinished }
			if active { continue }
			items.append(TransferItem(request: r, cameraReduced: cameraReduced, force: force))
			added += 1
		}
		save()
		pump()
		return added
	}

	public func cancel(_ id: UUID) {
		guard let i = items.firstIndex(where: { $0.id == id }) else { return }
		if currentID == id {
			userCancelled.insert(id)
			currentTask?.cancel()
		} else if !items[i].state.isFinished {
			items[i].state = .cancelled
			items[i].finishedAt = Date()
			engine.discardPartial(items[i].request.identity)
			save()
		}
	}

	public func cancelAll() {
		for item in items where !item.state.isFinished { cancel(item.id) }
	}

	public func retry(_ id: UUID) {
		guard let i = items.firstIndex(where: { $0.id == id }), items[i].state == .failed || items[i].state == .cancelled else { return }
		items[i].state = source == nil ? .waitingForConnection : .queued
		items[i].attempts = 0
		items[i].lastError = nil
		save()
		pump()
	}

	public func remove(_ id: UUID) {
		guard currentID != id else { return }
		items.removeAll { $0.id == id }
		save()
	}

	public func clearFinished() {
		items.removeAll { $0.state.isFinished }
		save()
	}

	// MARK: status for background task / UI

	public var pendingCount: Int { items.filter { !$0.state.isFinished }.count }
	public var activeItem: TransferItem? { items.first { $0.id == currentID } }

	/// Bytes done / total over unfinished items plus items finished in this run.
	public var aggregateProgress: (done: UInt64, total: UInt64) {
		var done: UInt64 = 0, total: UInt64 = 0
		for item in items where item.state != .cancelled {
			total += item.totalBytes
			done += item.state.isFinished ? item.totalBytes : item.bytesReceived
		}
		return (done, total)
	}

	// MARK: processing

	private func pump() {
		guard runner == nil, !isPaused, source != nil else { return }
		guard items.contains(where: { $0.state == .queued }) else { return }
		runner = Task { [weak self] in
			await self?.processLoop()
		}
	}

	private func processLoop() async {
		isProcessing = true
		startSampler()
		defer {
			isProcessing = false
			runner = nil
			sampler?.cancel()
			sampler = nil
			currentSpeed = 0
			onChange?()
		}
		while !isPaused, let source = self.source, let index = items.firstIndex(where: { $0.state == .queued }) {
			await process(index: index, source: source)
			onChange?()
		}
	}

	private func startSampler() {
		sampler?.cancel()
		sampler = Task { [weak self] in
			var lastBytes: UInt64 = 0
			var lastTime = Date()
			var lastID: UUID?
			while !Task.isCancelled {
				try? await Task.sleep(for: .milliseconds(250))
				guard let self, let id = self.currentID, let i = self.items.firstIndex(where: { $0.id == id }) else { continue }
				let (received, total) = self.progressBox.value
				if lastID != id { lastID = id; lastBytes = received; lastTime = Date() }
				let now = Date()
				let dt = now.timeIntervalSince(lastTime)
				if dt > 0, received >= lastBytes {
					let instant = Double(received - lastBytes) / dt
					self.currentSpeed = self.currentSpeed == 0 ? instant : self.currentSpeed * 0.7 + instant * 0.3
				}
				lastBytes = received
				lastTime = now
				if self.items[i].state == .running {
					self.items[i].bytesReceived = received
					if total > 0 { self.items[i].totalBytes = total }
				}
			}
		}
	}

	private func process(index: Int, source: any CameraObjectSource) async {
		let id = items[index].id
		currentID = id
		defer { currentID = nil }

		// Skip files already imported (same camera, name, size and capture time).
		if !items[index].force, case .alreadyImported(let record) = engine.store.status(for: items[index].request.identity) {
			items[index].state = .duplicate
			items[index].localRelativePath = record.localRelativePath
			items[index].notes.append("此前已导入（\(record.importedAt.formatted(date: .abbreviated, time: .shortened))），未重复下载")
			items[index].finishedAt = Date()
			save()
			return
		}

		items[index].state = .running
		items[index].attempts += 1
		items[index].lastError = nil
		progressBox.set((items[index].bytesReceived, items[index].totalBytes))
		save()

		await willStart?(items[index])
		guard let index = items.firstIndex(where: { $0.id == id }), items[index].state == .running else { return }
		let request = items[index].request
		var options = optionsProvider()
		options.cameraReduced = items[index].cameraReduced
		let box = progressBox
		let engine = self.engine
		let task = Task<TransferOutcome, Error> {
			try await engine.run(request, source: source, options: options) { received, total in
				box.set((received, total))
			}
		}
		currentTask = task
		let result = await task.result
		currentTask = nil
		guard let i = items.firstIndex(where: { $0.id == id }) else { return }

		switch result {
		case .success(let outcome):
			items[i].notes.append(contentsOf: outcome.notes)
			items[i].bytesReceived = items[i].totalBytes
			if outcome.duration > 0 { items[i].averageSpeed = Double(outcome.bytesTransferred) / outcome.duration }
			switch outcome.kind {
			case .duplicate(let existing):
				items[i].state = .duplicate
				items[i].localRelativePath = existing.localRelativePath
				items[i].finishedAt = Date()
			case .imported(let record):
				items[i].localRelativePath = record.localRelativePath
				if saveToPhotos, let saver, let url = outcome.fileURL {
					items[i].state = .saving
					save()
					do {
						let assetID = try await saver.save(fileAt: url, format: record.format, originalFilename: request.identity.filename, creationDate: request.dateCreated)
						engine.store.update(record.id) { $0.photosLocalIdentifier = assetID }
						items[i].photosStatus = .saved(assetID)
					} catch let e as PhotoSaveError where e == .notAuthorized {
						items[i].photosStatus = .denied
						items[i].notes.append("未获得照片添加权限，文件已保存在“文件”App 中")
					} catch {
						items[i].photosStatus = .failed(error.localizedDescription)
						items[i].notes.append("存入照片失败：\(error.localizedDescription)；文件已保存在“文件”App 中")
					}
				}
				items[i].state = .completed
				items[i].finishedAt = Date()
			}
		case .failure(let error):
			let failure = (error as? TransferFailure) ?? (error is CancellationError ? .cancelled : .writeFailed(error.localizedDescription))
			if userCancelled.remove(id) != nil {
				items[i].state = .cancelled
				items[i].finishedAt = Date()
				engine.discardPartial(request.identity)
			} else if failure == .cancelled || (failure.isRetryable && self.source == nil) {
				// Interrupted by detach (connection lost / backgrounded): keep the partial file.
				items[i].state = .waitingForConnection
				items[i].lastError = failure.localizedDescription
			} else if failure.isRetryable, items[i].attempts < maxAttempts {
				items[i].state = .waitingForConnection
				items[i].lastError = "\(failure.localizedDescription)（第 \(items[i].attempts) 次，重连后自动重试）"
			} else {
				items[i].state = .failed
				items[i].lastError = failure.localizedDescription
				items[i].finishedAt = Date()
			}
			if case .ptp(let e) = failure, e.isConnectionLost {
				// Stop the loop; the app re-attaches after reconnecting.
				self.source = nil
				for j in items.indices where items[j].state == .queued { items[j].state = .waitingForConnection }
			}
		}
		save()
	}

	private func save() {
		do {
			try JSONFile.save(items, to: fileURL)
		} catch {
			engine.logger.error("transfer", "保存传输队列失败：\(error.localizedDescription)")
		}
	}
}

public enum PhotoSaveError: Error, LocalizedError, Equatable, Sendable {
	case notAuthorized
	case unsupportedFormat(String)
	case failed(String)

	public var errorDescription: String? {
		switch self {
		case .notAuthorized: "未获得添加照片的权限"
		case .unsupportedFormat(let f): "“照片”不支持此格式（\(f)），文件仍保存在“文件”App 中"
		case .failed(let why): why
		}
	}
}
