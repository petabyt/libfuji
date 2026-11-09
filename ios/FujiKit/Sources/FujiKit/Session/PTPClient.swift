import Foundation
import CFujiCore

/// How the camera's Wi-Fi was started. Values equal libfuji's enum FujiTransport.
public enum CameraTransport: Int32, Sendable, Codable, CaseIterable {
	/// 'WIRELESS COMMUNICATION' started from the camera menu (Camera Remote era).
	case wirelessComm = 2
	/// Wi-Fi started through a Bluetooth handoff (XApp era). libfuji downgrades to
	/// wirelessComm when the camera reports remote_version < 0x20007.
	case xappWirelessComm = 9

	public var label: String {
		switch self {
		case .wirelessComm: "相机菜单发起的无线通信"
		case .xappWirelessComm: "蓝牙引导的 Wi-Fi（XApp 方式）"
		}
	}
}

/// Snapshot of libfuji's per-session state (struct PtpUserPriv).
public struct CameraSessionState: Sendable, Equatable {
	public var transport: CameraTransport?
	public var cameraStateRaw: Int32
	public var numObjects: Int32
	public var remoteVersion: Int32
	public var imageGetVersion: Int32
	public var getObjectVersion: Int32
	public var remoteImageViewVersion: Int32
	public var remoteImageViewExVersion: Int32
	public var sortOldestFirst: Bool
	public var liveviewSocketsOpen: Bool
	public var compressSmall: Bool
	public var storageInfoUpdates: UInt32
	public var pushedHandlesPending: UInt32
	public var cameraName: String
	public var storageName: String

	public var cameraState: FujiPTP.CameraState? { FujiPTP.CameraState(rawValue: cameraStateRaw) }
}

/// Receives a download on the PTP worker thread. Implementations must not
/// block for long: the camera session is busy until the call returns.
public protocol DownloadSink: AnyObject, Sendable {
	/// Called once before any data. Return false to refuse (e.g. not enough disk space).
	func begin(info: CameraObjectInfo, startOffset: UInt64) -> Bool
	/// Called for each chunk, in order. Return false to cancel the download.
	func write(_ bytes: UnsafeRawBufferPointer, at offset: UInt64, total: UInt64) -> Bool
}

/// Property values around a download (see fujiios_download_report).
public struct DownloadReport: Sendable, Equatable {
	public var compressSmallBefore: Int32?
	public var compressSmallAfter: Int32?
	public var correctSizeBefore: Int32?
	public var correctSizeAfter: Int32?
	public var expectedSize: UInt64
	public var bytesReceived: UInt64
	public var cleanupCode: Int32

	init(_ r: fujiios_download_report) {
		func v(_ x: Int32) -> Int32? { x == -2 ? nil : x }
		compressSmallBefore = v(r.compress_small_before)
		compressSmallAfter = v(r.compress_small_after)
		correctSizeBefore = v(r.correct_size_before)
		correctSizeAfter = v(r.correct_size_after)
		expectedSize = r.expected_size
		bytesReceived = r.bytes_received
		cleanupCode = r.cleanup_rc
	}

	public init(expectedSize: UInt64, bytesReceived: UInt64) {
		self.expectedSize = expectedSize
		self.bytesReceived = bytesReceived
		self.cleanupCode = 0
	}

	public var auditSummary: String {
		func f(_ v: Int32?) -> String { v.map { $0 < 0 ? "不可读" : "\($0)" } ?? "未检查" }
		return "D226 \(f(compressSmallBefore))→\(f(compressSmallAfter))，D227 \(f(correctSizeBefore))→\(f(correctSizeAfter))，恢复步骤 rc=\(cleanupCode)"
	}
}

/// What the transfer layer needs from a camera session (lets tests use a fake).
public protocol CameraObjectSource: AnyObject, Sendable {
	func objectHandles() async throws -> [UInt32]
	func objectInfo(handle: UInt32) async throws -> CameraObjectInfo
	func download(handle: UInt32, from offset: UInt64, audit: Bool, sink: any DownloadSink) async throws -> DownloadReport
	func readPartial(handle: UInt32, offset: UInt32, length: UInt32) async throws -> [UInt8]
}

/// Swift wrapper around one fujiios_session. All C calls run on `worker`.
public final class PTPClient: CameraObjectSource, @unchecked Sendable {
	struct Handle: @unchecked Sendable { let ptr: OpaquePointer }

	private final class LogContext: Sendable {
		let logger: FujiLogger
		init(logger: FujiLogger) { self.logger = logger }
	}

	/// The retained log context handed to C as `void *user`.
	private struct LogContextRef: @unchecked Sendable {
		let unmanaged: Unmanaged<LogContext>
	}

	private final class DownloadBridge {
		let sink: any DownloadSink
		let offset: UInt64
		init(sink: any DownloadSink, offset: UInt64) { self.sink = sink; self.offset = offset }
	}

	public let transport: CameraTransport
	public let worker: PTPWorker
	public let logger: FujiLogger
	private let handle: Handle
	private let logContext: LogContextRef

	public init(transport: CameraTransport, logger: FujiLogger = .shared) throws {
		fujiios_global_init()
		self.transport = transport
		self.logger = logger
		let context = Unmanaged.passRetained(LogContext(logger: logger))
		let callback: fujiios_log_fn = { user, level, message in
			guard let user, let message else { return }
			let ctx = Unmanaged<LogContext>.fromOpaque(user).takeUnretainedValue()
			let lvl: LogLevel = level >= 2 ? .error : (level == 1 ? .info : .verbose)
			ctx.logger.log(lvl, "ptp", String(cString: message))
		}
		guard let ptr = fujiios_session_new(transport.rawValue, callback, context.toOpaque(), Int32(FUJIIOS_LOG_VERBOSE)) else {
			context.release()
			throw PTPError.core(code: CoreRC.outOfMemory, responseCode: 0, operation: "创建会话")
		}
		self.handle = Handle(ptr: ptr)
		self.logContext = LogContextRef(unmanaged: context)
		self.worker = PTPWorker(name: "ptp-worker")
	}

	deinit {
		// Unblock anything still running, then free on the worker after it.
		let h = handle
		let ctx = logContext
		fujiios_abort(h.ptr)
		worker.cancelPending()
		worker.submit(.high) {
			fujiios_close(h.ptr, 0)
			fujiios_session_free(h.ptr)
			ctx.unmanaged.release()
		}
		worker.stop()
	}

	// MARK: helpers

	private static func check(_ h: Handle, _ rc: Int32, _ op: String) throws {
		guard rc == 0 else {
			throw PTPError.core(code: rc, responseCode: fujiios_last_response_code(h.ptr), operation: op)
		}
	}

	private static func take(_ b: inout fujiios_buffer) -> [UInt8] {
		defer { fujiios_buffer_free(&b) }
		guard let data = b.data, b.length > 0 else { return [] }
		return Array(UnsafeBufferPointer(start: data, count: Int(b.length)))
	}

	private func run<T: Sendable>(_ priority: PTPWorker.Priority = .normal, _ op: String,
								  _ body: @escaping @Sendable (Handle) throws -> T) async throws -> T {
		let h = handle
		do {
			return try await worker.run(priority) { try body(h) }
		} catch let e as PTPError {
			logger.log(e.isCancelled ? .info : .warning, "ptp", e.debugName)
			throw e
		}
	}

	// MARK: control (any thread)

	/// Shuts down the sockets so a blocked call returns. The session cannot be used afterwards.
	public func abort() {
		fujiios_abort(handle.ptr)
	}

	// MARK: connection

	/// Opens the PTP/IP command socket. With `requireCameraSubnet` the socket is
	/// bound (IP_BOUND_IF) to the interface on the camera's subnet, and the call
	/// fails early if no such interface exists (phone not on the camera Wi-Fi).
	public func connect(ip: String = FujiPTP.defaultCameraIP, port: Int32 = FujiPTP.commandPort,
						requireCameraSubnet: Bool = true, extraTimeout: Int32 = 1) async throws {
		var ifname = [CChar](repeating: 0, count: 32)
		var local = [CChar](repeating: 0, count: 64)
		let found = fujiios_find_interface_for_ipv4(ip, &ifname, ifname.count, &local, local.count) == 0
		let bind: String? = found ? String(decoding: ifname.prefix { $0 != 0 }.map { UInt8(bitPattern: $0) }, as: UTF8.self) : nil
		if requireCameraSubnet && !found {
			throw PTPError.notOnCameraNetwork(cameraIP: ip)
		}
		if found {
			let localIP = String(decoding: local.prefix { $0 != 0 }.map { UInt8(bitPattern: $0) }, as: UTF8.self)
			logger.info("ptp", "camera \(ip) reachable via \(bind ?? "?") local \(localIP)")
		}
		try await run(.high, "连接相机") { h in
			try PTPClient.check(h, fujiios_connect(h.ptr, ip, port, extraTimeout, bind), "连接相机")
		}
	}

	/// libfuji fuji_setup(). Blocks while the camera waits for the user to press
	/// OK; `timeout` aborts the session if that takes too long (upstream has no limit).
	public func setup(clientName: String, timeout: Duration = .seconds(120)) async throws {
		let timedOut = Locked(false)
		let watchdog = Task { [handle] in
			try await Task.sleep(for: timeout)
			timedOut.set(true)
			fujiios_abort(handle.ptr)
		}
		defer { watchdog.cancel() }
		do {
			try await withTaskCancellationHandler {
				try await run(.high, "初始化会话") { h in
					try PTPClient.check(h, fujiios_setup(h.ptr, clientName), "初始化会话")
				}
			} onCancel: { [handle] in
				fujiios_abort(handle.ptr)
			}
		} catch {
			if timedOut.value {
				throw PTPError.setupTimedOut(seconds: Int(timeout.components.seconds))
			}
			throw error
		}
	}

	public func close(graceful: Bool = true) async {
		_ = try? await run(.high, "断开") { h in
			_ = fujiios_close(h.ptr, graceful ? 1 : 0)
		}
	}

	public func isConnected() async -> Bool {
		(try? await run(.high, "") { h in fujiios_is_connected(h.ptr) != 0 }) ?? false
	}

	public func state() async throws -> CameraSessionState {
		try await run(.high, "读取状态") { h in
			var s = fujiios_state()
			try PTPClient.check(h, fujiios_get_state(h.ptr, &s), "读取状态")
			return CameraSessionState(
				transport: CameraTransport(rawValue: s.transport),
				cameraStateRaw: s.camera_state,
				numObjects: s.num_objects,
				remoteVersion: s.remote_version,
				imageGetVersion: s.image_get_version,
				getObjectVersion: s.get_object_version,
				remoteImageViewVersion: s.remote_image_view_version,
				remoteImageViewExVersion: s.remote_image_view_ex_version,
				sortOldestFirst: s.sort_by_oldest_first != 0,
				liveviewSocketsOpen: s.opened_liveview_sockets != 0,
				compressSmall: s.compress_small != 0,
				storageInfoUpdates: s.storage_info_updates,
				pushedHandlesPending: s.pushed_handles_pending,
				cameraName: withUnsafeBytes(of: &s.camera_name) { CString.string(from: $0) },
				storageName: withUnsafeBytes(of: &s.storage_name) { CString.string(from: $0) }
			)
		}
	}

	/// libfuji fuji_get_events(): updates camera state/object count.
	public func pollEvents(priority: PTPWorker.Priority = .background) async throws {
		try await run(priority, "读取相机事件") { h in
			try PTPClient.check(h, fujiios_poll_events(h.ptr), "读取相机事件")
		}
	}

	// MARK: objects

	public func objectHandles() async throws -> [UInt32] {
		try await run(.normal, "枚举文件") { h in
			var count: UInt32 = 0
			try PTPClient.check(h, fujiios_object_handles(h.ptr, nil, 0, &count), "枚举文件")
			guard count > 0 else { return [] }
			var out = [UInt32](repeating: 0, count: Int(count))
			var got: UInt32 = 0
			try PTPClient.check(h, fujiios_object_handles(h.ptr, &out, count, &got), "枚举文件")
			return Array(out.prefix(Int(min(got, count))))
		}
	}

	public func objectInfo(handle objectHandle: UInt32) async throws -> CameraObjectInfo {
		try await objectInfo(handle: objectHandle, priority: .normal)
	}

	public func objectInfo(handle objectHandle: UInt32, priority: PTPWorker.Priority) async throws -> CameraObjectInfo {
		try await run(priority, "读取文件信息") { h in
			var info = fujiios_object_info()
			try PTPClient.check(h, fujiios_get_object_info(h.ptr, objectHandle, &info), "读取文件信息")
			return CameraObjectInfo(info)
		}
	}

	/// Camera thumbnail (JPEG) or nil when the camera has none. Also returns the
	/// object info when libfuji had to fetch it first (newer cameras).
	public func thumbnail(handle objectHandle: UInt32, priority: PTPWorker.Priority = .normal) async throws -> (jpeg: Data?, info: CameraObjectInfo?) {
		try await run(priority, "读取缩略图") { h in
			var buffer = fujiios_buffer()
			var info = fujiios_object_info()
			var valid: Int32 = 0
			let rc = fujiios_get_thumbnail(h.ptr, objectHandle, &buffer, &info, &valid)
			let bytes = PTPClient.take(&buffer)
			try PTPClient.check(h, rc, "读取缩略图")
			return (bytes.isEmpty ? nil : Data(bytes), valid != 0 ? CameraObjectInfo(info) : nil)
		}
	}

	public func download(handle objectHandle: UInt32, from offset: UInt64, audit: Bool, sink: any DownloadSink) async throws -> DownloadReport {
		let cancelToken = Locked(false)
		return try await withTaskCancellationHandler {
			try await run(.high, "下载文件") { [cancelToken] h in
				fujiios_request_cancel(h.ptr, 0)
				if cancelToken.value { fujiios_request_cancel(h.ptr, 1) }
				let bridge = DownloadBridge(sink: sink, offset: offset)
				var callbacks = fujiios_download_callbacks()
				callbacks.user = Unmanaged.passUnretained(bridge).toOpaque()
				callbacks.on_info = { user, info in
					guard let user, let info else { return 1 }
					let b = Unmanaged<DownloadBridge>.fromOpaque(user).takeUnretainedValue()
					return b.sink.begin(info: CameraObjectInfo(info.pointee), startOffset: b.offset) ? 0 : 1
				}
				callbacks.on_data = { user, data, length, chunkOffset, total in
					guard let user, let data else { return 1 }
					let b = Unmanaged<DownloadBridge>.fromOpaque(user).takeUnretainedValue()
					return b.sink.write(UnsafeRawBufferPointer(start: data, count: Int(length)), at: chunkOffset, total: total) ? 0 : 1
				}
				var report = fujiios_download_report()
				let rc = withExtendedLifetime(bridge) {
					fujiios_download(h.ptr, objectHandle, offset, audit ? 1 : 0, &callbacks, &report)
				}
				fujiios_request_cancel(h.ptr, 0)
				try PTPClient.check(h, rc, "下载文件")
				return DownloadReport(report)
			}
		} onCancel: { [handle] in
			cancelToken.set(true)
			fujiios_request_cancel(handle.ptr, 1)
		}
	}

	public func readPartial(handle objectHandle: UInt32, offset: UInt32, length: UInt32) async throws -> [UInt8] {
		try await run(.high, "读取文件片段") { h in
			var buffer = fujiios_buffer()
			let rc = fujiios_read_partial(h.ptr, objectHandle, offset, length, &buffer)
			let bytes = PTPClient.take(&buffer)
			try PTPClient.check(h, rc, "读取文件片段")
			return bytes
		}
	}

	/// XApp-mode "resize before download" flag (libfuji compress_images_for_smartphone).
	public func setCompressSmall(_ on: Bool) async {
		_ = try? await run(.high, "") { h in fujiios_set_compress_small(h.ptr, on ? 1 : 0) }
	}

	public func takePushedHandles() async throws -> [UInt32] {
		try await run(.normal, "读取相机推送") { h in
			var out = [UInt32](repeating: 0, count: 64)
			var count: UInt32 = 0
			try PTPClient.check(h, fujiios_take_pushed_handles(h.ptr, &out, 64, &count), "读取相机推送")
			return Array(out.prefix(Int(count)))
		}
	}

	// MARK: liveview

	public func liveviewStart() async throws {
		try await run(.high, "进入实时取景") { h in
			try PTPClient.check(h, fujiios_liveview_start(h.ptr), "进入实时取景")
		}
	}

	public func liveviewStop() async throws {
		try await run(.high, "退出实时取景") { h in
			try PTPClient.check(h, fujiios_liveview_stop(h.ptr), "退出实时取景")
		}
	}

	/// One JPEG frame, or nil when none was ready.
	public func liveviewFrame() async throws -> Data? {
		try await run(.normal, "读取取景帧") { h in
			var buffer = fujiios_buffer()
			let rc = fujiios_liveview_frame(h.ptr, &buffer)
			let bytes = PTPClient.take(&buffer)
			try PTPClient.check(h, rc, "读取取景帧")
			return bytes.isEmpty ? nil : Data(bytes)
		}
	}

	// MARK: properties and info

	public func propertyValueRaw(_ code: UInt16) async throws -> [UInt8] {
		try await run(.normal, String(format: "读取属性 0x%04X", code)) { h in
			var buffer = fujiios_buffer()
			let rc = fujiios_get_prop_raw(h.ptr, code, &buffer)
			let bytes = PTPClient.take(&buffer)
			try PTPClient.check(h, rc, String(format: "读取属性 0x%04X", code))
			return bytes
		}
	}

	/// Parsed descriptor, or nil if the camera does not support the property.
	public func propertyDescriptor(_ code: UInt16) async throws -> PropertyDescriptor? {
		let bytes: [UInt8]
		do {
			bytes = try await run(.normal, String(format: "读取属性描述 0x%04X", code)) { h in
				var buffer = fujiios_buffer()
				let rc = fujiios_get_prop_desc_raw(h.ptr, code, &buffer)
				let bytes = PTPClient.take(&buffer)
				try PTPClient.check(h, rc, String(format: "读取属性描述 0x%04X", code))
				return bytes
			}
		} catch let e as PTPError where e.coreCode == CoreRC.checkCode {
			return nil
		}
		do {
			let d = try PropertyDescriptor.parse(bytes)
			guard d.code == code else { throw PTPError.invalidPayload(String(format: "描述的属性码 0x%04X 不符", d.code)) }
			return d
		} catch let e as ByteReader.Error {
			throw PTPError.invalidPayload("属性描述 \(String(format: "0x%04X", code))：\(e)")
		}
	}

	public func setPropertyRaw(_ code: UInt16, bytes: [UInt8]) async throws {
		try await run(.high, String(format: "设置属性 0x%04X", code)) { h in
			let rc = bytes.withUnsafeBufferPointer { fujiios_set_prop_raw(h.ptr, code, $0.baseAddress, UInt32($0.count)) }
			try PTPClient.check(h, rc, String(format: "设置属性 0x%04X", code))
		}
	}

	public func deviceInfo() async throws -> PTPDeviceInfo {
		let bytes: [UInt8] = try await run(.normal, "读取设备信息") { h in
			var buffer = fujiios_buffer()
			let rc = fujiios_device_info_raw(h.ptr, &buffer)
			let bytes = PTPClient.take(&buffer)
			try PTPClient.check(h, rc, "读取设备信息")
			return bytes
		}
		do { return try PTPDeviceInfo.parse(bytes) } catch { throw PTPError.invalidPayload("DeviceInfo：\(error)") }
	}

	public func storageInfos() async throws -> [(id: UInt32, info: PTPStorageInfo)] {
		let ids: [UInt32] = try await run(.normal, "读取存储列表") { h in
			var buffer = fujiios_buffer()
			let rc = fujiios_storage_ids_raw(h.ptr, &buffer)
			let bytes = PTPClient.take(&buffer)
			try PTPClient.check(h, rc, "读取存储列表")
			var r = ByteReader(bytes)
			return (try? r.u32Array(limit: 16)) ?? []
		}
		var result: [(UInt32, PTPStorageInfo)] = []
		for id in ids {
			let bytes: [UInt8] = try await run(.normal, "读取存储信息") { h in
				var buffer = fujiios_buffer()
				let rc = fujiios_storage_info_raw(h.ptr, id, &buffer)
				let bytes = PTPClient.take(&buffer)
				try PTPClient.check(h, rc, "读取存储信息")
				return bytes
			}
			if let info = try? PTPStorageInfo.parse(bytes) { result.append((id, info)) }
		}
		return result
	}
}
