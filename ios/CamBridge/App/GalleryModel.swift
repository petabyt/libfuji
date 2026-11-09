import Foundation
import Observation
import FujiKit

/// Camera gallery: handles listed by libfuji (1…N over Wi-Fi), object info
/// fetched progressively at background priority, thumbnails on demand.
@MainActor
@Observable
final class GalleryModel {
	enum ThumbState: Equatable { case notLoaded, loading, loaded, none, failed(String) }

	enum Filter: String, CaseIterable, Identifiable {
		case all = "全部", jpeg = "JPEG", heif = "HEIF", raf = "RAF", video = "视频"
		var id: String { rawValue }
		func matches(_ f: MediaFormat?) -> Bool {
			guard let f else { return self == .all }
			switch self {
			case .all: return true
			case .jpeg: return f == .jpeg
			case .heif: return f == .heif
			case .raf: return f == .raf
			case .video: return f.isVideo
			}
		}
	}

	struct Item: Identifiable, Equatable {
		let handle: UInt32
		var info: CameraObjectInfo?
		var thumbnail: Data?
		var thumbState: ThumbState = .notLoaded
		var importStatus: DuplicateStatus = .new
		var infoError: String?
		var id: UInt32 { handle }
	}

	private(set) var items: [Item] = []
	var newestFirst = true
	var filter: Filter = .all
	var selection: Set<UInt32> = []
	private(set) var isLoading = false
	private(set) var infoLoaded = 0
	private(set) var message: String?
	private(set) var cameraOrderOldestFirst = false
	private(set) var isOffline = true

	@ObservationIgnored private var client: PTPClient?
	@ObservationIgnored private var cameraID = ""
	@ObservationIgnored private var imports: ImportStore?
	@ObservationIgnored private var infoTask: Task<Void, Never>?
	@ObservationIgnored private let thumbCache = NSCache<NSString, NSData>()

	var totalCount: Int { items.count }

	var displayed: [Item] {
		let filtered = items.filter { $0.info?.isFolder != true && filter.matches($0.info?.format) }
		// Camera order: libfuji sets sort_by_oldest_first for newer cameras (handle 1 = oldest).
		let cameraNewestFirst = !cameraOrderOldestFirst
		return filtered.sorted { a, b in
			if let da = a.info?.dateCreated, let db = b.info?.dateCreated, da != db {
				return newestFirst ? da > db : da < db
			}
			let aHas = a.info?.dateCreated != nil, bHas = b.info?.dateCreated != nil
			if aHas != bHas { return aHas } // items with a known date first
			let handleNewestFirst = newestFirst == cameraNewestFirst
			return handleNewestFirst ? a.handle < b.handle : a.handle > b.handle
		}
	}

	func item(_ handle: UInt32) -> Item? { items.first { $0.handle == handle } }

	func reset(offlineMessage: String?) {
		infoTask?.cancel()
		client = nil
		isOffline = true
		message = offlineMessage
	}

	func reload(client: PTPClient, state: CameraSessionState, cameraID: String, imports: ImportStore) async {
		infoTask?.cancel()
		self.client = client
		self.cameraID = cameraID
		self.imports = imports
		cameraOrderOldestFirst = state.sortOldestFirst
		isOffline = false
		isLoading = true
		message = nil
		defer { isLoading = false }
		do {
			let handles = try await client.objectHandles()
			let old = Dictionary(uniqueKeysWithValues: items.map { ($0.handle, $0) })
			items = handles.map { h in
				// Handles are positions; reuse cached data only if the identity still matches (checked below).
				var i = Item(handle: h)
				if let prev = old[h], prev.info != nil { i.info = prev.info; i.thumbnail = prev.thumbnail; i.thumbState = prev.thumbState }
				return i
			}
			selection = selection.filter { h in items.contains { $0.handle == h } }
			infoLoaded = 0
			if items.isEmpty { message = "存储卡上没有可浏览的文件" }
			startInfoPrefetch()
		} catch {
			message = error.localizedDescription
			items = []
		}
	}

	/// Fetches object info for every handle in display order at background priority,
	/// so user actions and thumbnails go first on the PTP worker.
	private func startInfoPrefetch() {
		infoTask = Task { [weak self] in
			guard let self else { return }
			let order = self.displayed.map(\.handle)
			for h in order {
				if Task.isCancelled { return }
				await self.loadInfo(h, priority: .background)
			}
		}
	}

	func loadInfo(_ handle: UInt32, priority: PTPWorker.Priority = .normal) async {
		guard let client else { return }
		do {
			let info = try await client.objectInfo(handle: handle, priority: priority)
			apply(info: info, to: handle)
		} catch let e as PTPError where e.isConnectionLost {
			infoTask?.cancel()
		} catch {
			if let i = items.firstIndex(where: { $0.handle == handle }) { items[i].infoError = error.localizedDescription }
		}
	}

	private func apply(info: CameraObjectInfo, to handle: UInt32) {
		guard let i = items.firstIndex(where: { $0.handle == handle }) else { return }
		if let old = items[i].info, ObjectIdentity(cameraID: cameraID, info: old) != ObjectIdentity(cameraID: cameraID, info: info) {
			// The handle now points at another file: drop the stale thumbnail.
			items[i].thumbnail = nil
			items[i].thumbState = .notLoaded
		}
		let wasMissing = items[i].info == nil
		items[i].info = info
		items[i].infoError = nil
		if let imports { items[i].importStatus = imports.status(for: ObjectIdentity(cameraID: cameraID, info: info)) }
		if wasMissing { infoLoaded += 1 }
		if let cached = thumbCache.object(forKey: cacheKey(info) as NSString) {
			items[i].thumbnail = cached as Data
			items[i].thumbState = .loaded
		}
	}

	private func cacheKey(_ info: CameraObjectInfo) -> String { ObjectIdentity(cameraID: cameraID, info: info).key }

	func loadThumbnail(_ handle: UInt32) async {
		guard let client, let i = items.firstIndex(where: { $0.handle == handle }), items[i].thumbState == .notLoaded else { return }
		items[i].thumbState = .loading
		do {
			let result = try await client.thumbnail(handle: handle, priority: .normal)
			if let info = result.info { apply(info: info, to: handle) }
			guard let j = items.firstIndex(where: { $0.handle == handle }) else { return }
			if let jpeg = result.jpeg {
				items[j].thumbnail = jpeg
				items[j].thumbState = .loaded
				if let info = items[j].info { thumbCache.setObject(jpeg as NSData, forKey: cacheKey(info) as NSString) }
			} else {
				items[j].thumbState = .none
			}
			if items[j].info == nil { await loadInfo(handle) }
		} catch let e as PTPError where e.isConnectionLost {
			if let j = items.firstIndex(where: { $0.handle == handle }) { items[j].thumbState = .notLoaded }
		} catch {
			if let j = items.firstIndex(where: { $0.handle == handle }) { items[j].thumbState = .failed(error.localizedDescription) }
		}
	}

	func refreshImportStatus() {
		guard let imports else { return }
		for i in items.indices {
			if let info = items[i].info { items[i].importStatus = imports.status(for: ObjectIdentity(cameraID: cameraID, info: info)) }
		}
	}

	/// Transfer requests for the selection; fetches missing info first.
	func requests(for handles: [UInt32], cameraName: String) async -> [TransferRequest] {
		var out: [TransferRequest] = []
		for h in handles {
			if item(h)?.info == nil { await loadInfo(h, priority: .high) }
			guard let info = item(h)?.info, !info.isFolder else { continue }
			out.append(TransferRequest(identity: ObjectIdentity(cameraID: cameraID, info: info), lastKnownHandle: h,
									   cameraDisplayName: cameraName, format: info.format, dateCreated: info.dateCreated))
		}
		return out
	}
}
