#if canImport(ImageCaptureCore) && os(iOS)
import Foundation
import ImageCaptureCore

/// USB import through ImageCaptureCore, the public framework iOS offers for
/// cameras connected by cable. Third-party apps cannot open arbitrary USB
/// endpoints on iOS, so libpict's libusb backend is not used. Scope:
/// - browsing and reading files of a camera that presents itself as a PTP/MTP
///   camera (Fujifilm "USB CARD READER" connection mode is expected to);
/// - a read-only PTP pass-through diagnostic (GetDeviceInfo and the USB mode
///   property 0xD16E) to find out what mode the camera exposes.
/// Tethering, RAW conversion and backup/restore over USB are not implemented.
public final class USBCameraBrowser: NSObject, ICDeviceBrowserDelegate, ICCameraDeviceDelegate, @unchecked Sendable {
	public struct DeviceSummary: Sendable, Identifiable, Equatable {
		public let id: String
		public let name: String
		public let vendorID: Int32
		public let productID: Int32
		public let hasOpenSession: Bool
		public let catalogReady: Bool
		public let fileCount: Int
		public var isFujifilm: Bool { vendorID == 0x04CB }
	}

	public enum Event: Sendable {
		case devices([DeviceSummary])
		case status(String)
	}

	public let events: AsyncStream<Event>
	private let sink: AsyncStream<Event>.Continuation
	private let browser = ICDeviceBrowser()
	private let lock = NSLock()
	private var cameras: [String: ICCameraDevice] = [:]
	private var ready: Set<String> = []
	private let logger: FujiLogger

	public init(logger: FujiLogger = .shared) {
		self.logger = logger
		var s: AsyncStream<Event>.Continuation!
		events = AsyncStream(bufferingPolicy: .bufferingNewest(32)) { s = $0 }
		sink = s
		super.init()
		browser.delegate = self
		browser.browsedDeviceTypeMask = ICDeviceTypeMask(rawValue: ICDeviceTypeMask.camera.rawValue | ICDeviceLocationTypeMask.local.rawValue) ?? .camera
	}

	/// Asks for access to camera contents (system prompt), then starts browsing.
	public func start() async -> Bool {
		let status = await browser.requestContentsAuthorization()
		guard status == .authorized else {
			sink.yield(.status("未获得访问相机内容的权限"))
			return false
		}
		browser.start()
		sink.yield(.status("正在查找通过 USB 连接的相机…"))
		return true
	}

	public func stop() {
		browser.stop()
		lock.withLock {
			for c in cameras.values where c.hasOpenSession { c.requestCloseSession() }
		}
	}

	public func open(_ id: String) {
		guard let camera = lock.withLock({ cameras[id] }) else { return }
		camera.delegate = self
		camera.requestOpenSession()
	}

	/// File source for the transfer engine once the content catalog is complete.
	public func source(for id: String) -> (any CameraObjectSource)? {
		lock.withLock {
			guard let camera = cameras[id], ready.contains(id) else { return nil }
			let files = (camera.mediaFiles ?? []).compactMap { $0 as? ICCameraFile }
			return ICCameraObjectSource(files: files)
		}
	}

	/// Read-only PTP pass-through: GetDeviceInfo and, if listed, the Fujifilm
	/// USB mode property 0xD16E (5 = tether, 6 = RAW conv., 8 = webcam per libfuji).
	public func ptpDiagnostics(for id: String) async throws -> (info: PTPDeviceInfo, usbMode: Int64?) {
		guard let camera = lock.withLock({ cameras[id] }) else { throw PTPError.notConnected }
		let control = await browser.requestControlAuthorization()
		guard control == .authorized else { throw PTPError.invalidPayload("未获得控制相机的权限") }
		let (_, infoData) = try await camera.requestSendPTPCommand(Self.commandContainer(code: 0x1001, params: []), outData: nil)
		let info = try PTPDeviceInfo.parse([UInt8](Self.stripDataContainer(infoData)))
		var usbMode: Int64?
		if info.supports(property: FujiPTP.Property.usbMode) {
			let (_, value) = try await camera.requestSendPTPCommand(Self.commandContainer(code: 0x1015, params: [UInt32(FujiPTP.Property.usbMode)]), outData: nil)
			let bytes = [UInt8](Self.stripDataContainer(value))
			if bytes.count >= 2 { usbMode = Int64(bytes[0]) | Int64(bytes[1]) << 8 }
		}
		return (info, usbMode)
	}

	/// USB-style PTP command container (length, type 1, code, transaction id, params).
	static func commandContainer(code: UInt16, params: [UInt32]) -> Data {
		var w = ByteWriter()
		w.u32(UInt32(12 + 4 * params.count))
		w.u16(1)
		w.u16(code)
		w.u32(0) // ImageCaptureCore manages the transaction id
		params.forEach { w.u32($0) }
		return w.data
	}

	/// The data phase may or may not include its 12-byte container header.
	static func stripDataContainer(_ data: Data) -> Data {
		let b = [UInt8](data)
		guard b.count >= 12 else { return data }
		let length = Int(UInt32(b[0]) | UInt32(b[1]) << 8 | UInt32(b[2]) << 16 | UInt32(b[3]) << 24)
		let type = UInt16(b[4]) | UInt16(b[5]) << 8
		return (length == b.count && type == 2) ? data.subdata(in: 12..<data.count) : data
	}

	private func publish() {
		let summaries = lock.withLock {
			cameras.map { id, c in
				DeviceSummary(id: id, name: c.name ?? "USB 相机", vendorID: c.usbVendorID, productID: c.usbProductID,
							  hasOpenSession: c.hasOpenSession, catalogReady: ready.contains(id),
							  fileCount: c.mediaFiles?.count ?? 0)
			}
		}
		sink.yield(.devices(summaries.sorted { $0.name < $1.name }))
	}

	// MARK: ICDeviceBrowserDelegate

	public func deviceBrowser(_ browser: ICDeviceBrowser, didAdd device: ICDevice, moreComing: Bool) {
		guard let camera = device as? ICCameraDevice else { return }
		let id = camera.uuidString ?? UUID().uuidString
		lock.withLock { cameras[id] = camera }
		logger.info("usb", "发现 USB 相机 \(camera.name ?? "?") VID=\(String(format: "0x%04X", camera.usbVendorID)) PID=\(String(format: "0x%04X", camera.usbProductID))")
		publish()
	}

	public func deviceBrowser(_ browser: ICDeviceBrowser, didRemove device: ICDevice, moreGoing: Bool) {
		lock.withLock {
			if let key = cameras.first(where: { $0.value === device })?.key {
				cameras[key] = nil
				ready.remove(key)
			}
		}
		publish()
	}

	// MARK: ICDeviceDelegate

	public func device(_ device: ICDevice, didOpenSessionWithError error: (any Error)?) {
		if let error { sink.yield(.status("打开相机会话失败：\(error.localizedDescription)")) }
		publish()
	}

	public func device(_ device: ICDevice, didCloseSessionWithError error: (any Error)?) {
		publish()
	}

	public func didRemove(_ device: ICDevice) {
		publish()
	}

	// MARK: ICCameraDeviceDelegate

	public func deviceDidBecomeReady(withCompleteContentCatalog device: ICCameraDevice) {
		lock.withLock {
			if let key = cameras.first(where: { $0.value === device })?.key { ready.insert(key) }
		}
		sink.yield(.status("已读取相机文件目录（\(device.mediaFiles?.count ?? 0) 个文件）"))
		publish()
	}

	public func cameraDevice(_ camera: ICCameraDevice, didAdd items: [ICCameraItem]) { publish() }
	public func cameraDevice(_ camera: ICCameraDevice, didRemove items: [ICCameraItem]) { publish() }
	public func cameraDevice(_ camera: ICCameraDevice, didRenameItems items: [ICCameraItem]) {}
	public func cameraDevice(_ camera: ICCameraDevice, didReceiveMetadata metadata: [AnyHashable: Any]?, for item: ICCameraItem, error: (any Error)?) {}
	public func cameraDevice(_ camera: ICCameraDevice, didReceiveThumbnail thumbnail: CGImage?, for item: ICCameraItem, error: (any Error)?) {}
	public func cameraDeviceDidChangeCapability(_ camera: ICCameraDevice) {}
	public func cameraDeviceDidEnableAccessRestriction(_ device: ICDevice) {
		sink.yield(.status("相机已锁定访问（请在相机上确认或解锁）"))
	}
	public func cameraDeviceDidRemoveAccessRestriction(_ device: ICDevice) { publish() }
	public func cameraDevice(_ camera: ICCameraDevice, didReceivePTPEvent eventData: Data) {}
}

/// Presents ImageCaptureCore files through the same interface as the Wi-Fi
/// PTP session so the transfer engine (identity, resume, dedup, Photos) is shared.
final class ICCameraObjectSource: CameraObjectSource, @unchecked Sendable {
	private let files: [ICCameraFile]

	init(files: [ICCameraFile]) { self.files = files }

	private func file(_ handle: UInt32) throws -> ICCameraFile {
		let i = Int(handle) - 1
		guard files.indices.contains(i) else { throw PTPError.core(code: CoreRC.checkCode, responseCode: 0x2009, operation: "USB 文件") }
		return files[i]
	}

	func objectHandles() async throws -> [UInt32] { files.indices.map { UInt32($0 + 1) } }

	func objectInfo(handle: UInt32) async throws -> CameraObjectInfo {
		let f = try file(handle)
		let name = f.name ?? f.originalFilename ?? "未命名"
		let date = f.exifCreationDate ?? f.fileCreationDate ?? f.creationDate
		let raw: String = date.map {
			let fmt = DateFormatter()
			fmt.locale = Locale(identifier: "en_US_POSIX")
			fmt.dateFormat = "yyyyMMdd'T'HHmmss"
			return fmt.string(from: $0)
		} ?? ""
		let format: UInt16 = MediaFormat.guess(objectFormat: 0, filename: name) == .jpeg ? 0x3801 : 0x3000
		return CameraObjectInfo(objectFormat: format, compressedSize: UInt32(clamping: f.fileSize), imageWidth: UInt32(clamping: max(0, f.width)),
								imageHeight: UInt32(clamping: max(0, f.height)), filename: name, dateCreatedRaw: raw)
	}

	func download(handle: UInt32, from offset: UInt64, audit: Bool, sink: any DownloadSink) async throws -> DownloadReport {
		let f = try file(handle)
		let info = try await objectInfo(handle: handle)
		guard sink.begin(info: info, startOffset: offset) else {
			throw PTPError.core(code: CoreRC.canceled, responseCode: 0, operation: "USB 下载")
		}
		let total = UInt64(max(0, f.fileSize))
		var position = offset
		while position < total {
			if Task.isCancelled { throw PTPError.core(code: CoreRC.canceled, responseCode: 0, operation: "USB 下载") }
			let length = min(UInt64(FujiPTP.maxPartialObject), total - position)
			let data = try await f.requestReadData(atOffset: off_t(position), length: off_t(length))
			guard !data.isEmpty else { throw PTPError.core(code: CoreRC.runtime, responseCode: 0, operation: "USB 读取返回空数据") }
			let ok = data.withUnsafeBytes { sink.write($0, at: position, total: total) }
			guard ok else { throw PTPError.core(code: CoreRC.canceled, responseCode: 0, operation: "USB 下载") }
			position += UInt64(data.count)
		}
		return DownloadReport(expectedSize: total, bytesReceived: position)
	}

	func readPartial(handle: UInt32, offset: UInt32, length: UInt32) async throws -> [UInt8] {
		let f = try file(handle)
		return [UInt8](try await f.requestReadData(atOffset: off_t(offset), length: off_t(length)))
	}
}
#endif
