import Foundation
import Observation
import SwiftUI
import UIKit
import Photos
import FujiKit

/// State of one link, shown independently for Bluetooth, Wi-Fi and the PTP session.
enum LinkStatus: Equatable {
	case idle(String)
	case working(String)
	case ready(String)
	case failed(String)

	var text: String {
		switch self {
		case .idle(let s), .working(let s), .ready(let s), .failed(let s): s
		}
	}

	var isReady: Bool { if case .ready = self { return true } else { return false } }
	var isWorking: Bool { if case .working = self { return true } else { return false } }
}

/// Information for the manual Wi-Fi join sheet.
struct ManualJoinRequest: Identifiable, Equatable {
	let id = UUID()
	var ssid: String?
	var password: String?
	var reason: String
	var transport: CameraTransport
}

struct AppAlert: Identifiable {
	let id = UUID()
	let title: String
	let message: String
}

@MainActor
@Observable
final class AppModel {
	// MARK: infrastructure
	@ObservationIgnored let logger = FujiLogger.shared
	@ObservationIgnored let paths: AppPaths
	let settings: AppSettings
	let pairings: PairingStore
	@ObservationIgnored let imports: ImportStore
	let queue: TransferQueue
	let gallery = GalleryModel()
	let location = LocationProvider()
	@ObservationIgnored let ble: BLECameraLink
	@ObservationIgnored let joiner: HotspotJoiner
	@ObservationIgnored let background: BackgroundTransferCoordinator
	@ObservationIgnored let engine: TransferEngine

	// MARK: link state
	var bluetoothAvailability: BluetoothAvailability = .unknown
	var bluetoothLink: LinkStatus = .idle("未连接")
	var wifiLink: LinkStatus = .idle("未连接")
	var sessionLink: LinkStatus = .idle("未建立")
	var discovered: [DiscoveredCamera] = []
	var isScanning = false
	var bleConnectedCamera: UUID?
	var bleDevice: BLEDeviceInformation?
	var lastTimeSync: Date?
	var lastGeotag: Date?
	var manualJoin: ManualJoinRequest?
	var alert: AppAlert?

	// MARK: session state
	@ObservationIgnored private(set) var client: PTPClient?
	var sessionState: CameraSessionState?
	var capabilities: CameraCapabilities = .disconnected
	var ptpDeviceInfo: PTPDeviceInfo?
	var batteryText: String?
	var storageText: String?
	var cameraID = ""

	// MARK: liveview / remote
	var liveviewActive = false
	var liveFrame: Data?
	var liveFrameCount = 0
	var liveviewError: String?

	@ObservationIgnored private var eventTask: Task<Void, Never>?
	@ObservationIgnored private var liveTask: Task<Void, Never>?
	@ObservationIgnored private var bleEventsTask: Task<Void, Never>?

	init() {
		// Use locals until every stored property is initialised (no `self` reads before that).
		let log = FujiLogger.shared
		let paths = AppPaths.standard()
		try? paths.ensureDirectories()
		self.paths = paths
		let settings = AppSettings(url: paths.settingsFile)
		self.settings = settings
		log.verboseEnabled = settings.verboseLogs
		pairings = PairingStore(fileURL: paths.pairingsFile, secrets: KeychainSecretStore(), redactor: log.redactor)
		let imports = ImportStore(fileURL: paths.importRecordsFile)
		self.imports = imports
		let engine = TransferEngine(paths: paths, store: imports, logger: log)
		self.engine = engine
		let queue = TransferQueue(engine: engine, fileURL: paths.transferQueueFile)
		self.queue = queue
		background = BackgroundTransferCoordinator(queue: queue, logger: log)
		ble = BLECameraLink(logger: log)
		joiner = HotspotJoiner(logger: log)

		queue.saver = PhotoLibrarySaver()
		queue.saveToPhotos = settings.saveToPhotos
		queue.optionsProvider = { [weak settings] in
			var o = TransferOptions()
			o.auditProperties = settings?.auditProperties ?? false
			return o
		}
		queue.willStart = { [weak self] item in
			// libfuji's resize flag is per session; set it to what this item was queued with.
			await self?.client?.setCompressSmall(item.cameraReduced)
		}
		queue.onChange = { [weak self] in self?.transferStateChanged() }
		ble.setTimeSyncEnabled(settings.timeSync)
		if settings.locationSync { location.start() }

		bleEventsTask = Task { [weak self] in
			guard let events = self?.ble.events else { return }
			for await event in events {
				self?.handle(event)
			}
		}
	}

	var activeCamera: PairedCamera? { pairings.activeCamera }

	var cameraDisplayName: String {
		if let n = sessionState?.cameraName, !n.isEmpty { return n }
		return activeCamera?.displayName ?? "相机"
	}

	// MARK: Bluetooth

	private func handle(_ event: BLEEvent) {
		switch event {
		case .availability(let a):
			bluetoothAvailability = a
			if a != .ready {
				isScanning = false
				bleConnectedCamera = nil
				bluetoothLink = .failed(a.label)
			} else if case .failed = bluetoothLink {
				bluetoothLink = .idle("未连接")
			}
		case .discovered(let camera):
			if let i = discovered.firstIndex(where: { $0.id == camera.id }) {
				discovered[i] = camera
			} else {
				discovered.append(camera)
			}
		case .connected:
			break
		case .disconnected(let id, let message):
			if bleConnectedCamera == id { bleConnectedCamera = nil }
			if let message {
				bluetoothLink = .failed("蓝牙已断开：\(message)")
			} else {
				bluetoothLink = .idle("已断开")
			}
			if pairings.camera(id) != nil {
				pairings.record(id, ConnectionRecord(link: .bluetooth, success: false, detail: message ?? "断开连接"))
			}
		case .notification:
			break
		case .geotagRequested:
			sendGeotagIfAllowed()
		case .timeSynced(let date):
			lastTimeSync = date
		}
	}

	func startScan() {
		discovered.removeAll()
		isScanning = true
		ble.startScan()
	}

	func stopScan() {
		isScanning = false
		ble.stopScan()
	}

	/// First pairing with a camera found by scanning, or re-pairing.
	func pair(with camera: DiscoveredCamera) async {
		stopScan()
		let existing = pairings.camera(camera.id)
		bluetoothLink = .working(camera.advertisement.schemeHint == .secureBond
			? "正在配对：请在相机上确认，并在 iPhone 弹出的蓝牙配对请求中点按“配对”"
			: "正在配对 \(camera.name)…")
		do {
			let result = try await ble.connectAndPair(
				peripheralID: camera.id, advertisement: camera.advertisement, savedScheme: existing?.scheme,
				legacyToken: existing.flatMap { pairings.secret(.legacyToken, for: $0.id) }.map { [UInt8]($0) },
				secureSerial: existing.flatMap { pairings.secret(.secureSerial, for: $0.id) }.map { [UInt8]($0) },
				clientName: settings.clientName)
			storePairing(result)
		} catch {
			bluetoothLink = .failed(error.localizedDescription)
			if existing != nil {
				pairings.record(camera.id, ConnectionRecord(link: .bluetooth, success: false, detail: error.localizedDescription))
			}
			logger.error("ble", "配对失败：\(error.localizedDescription)")
		}
	}

	/// Reconnects a saved camera with its stored scheme and secrets.
	func reconnect(_ camera: PairedCamera) async {
		pairings.setActive(camera.id)
		bluetoothLink = .working("正在连接 \(camera.displayName)…（请打开相机电源）")
		do {
			let result = try await ble.connectAndPair(
				peripheralID: camera.id, advertisement: discovered.first { $0.id == camera.id }?.advertisement,
				savedScheme: camera.scheme,
				legacyToken: pairings.secret(.legacyToken, for: camera.id).map { [UInt8]($0) },
				secureSerial: pairings.secret(.secureSerial, for: camera.id).map { [UInt8]($0) },
				clientName: settings.clientName)
			storePairing(result)
		} catch {
			bluetoothLink = .failed(error.localizedDescription)
			pairings.record(camera.id, ConnectionRecord(link: .bluetooth, success: false, detail: error.localizedDescription))
		}
	}

	private func storePairing(_ r: PairingResult) {
		var camera = pairings.camera(r.peripheralID) ?? PairedCamera(id: r.peripheralID, displayName: r.name, scheme: r.scheme)
		camera.displayName = r.name
		camera.scheme = r.scheme
		camera.bleModel = r.device.model ?? camera.bleModel
		camera.bleFirmware = r.device.firmware ?? camera.bleFirmware
		pairings.upsert(camera)
		do {
			if let t = r.legacyToken { try pairings.setSecret(Data(t), .legacyToken, for: r.peripheralID) }
			if let s = r.secureSerial { try pairings.setSecret(Data(s), .secureSerial, for: r.peripheralID) }
		} catch {
			alert = AppAlert(title: "保存配对信息失败", message: error.localizedDescription)
		}
		pairings.setActive(r.peripheralID)
		pairings.record(r.peripheralID, ConnectionRecord(link: .bluetooth, success: true, detail: r.scheme.label))
		bleConnectedCamera = r.peripheralID
		bleDevice = r.device
		var text = "已连接 \(r.name)（\(r.scheme.label)）"
		if !r.skippedSubscriptions.isEmpty { text += "，\(r.skippedSubscriptions.count) 项可选通知未订阅" }
		bluetoothLink = .ready(text)
		logger.info("ble", text)
	}

	func disconnectBluetooth() async {
		await ble.disconnect()
		bleConnectedCamera = nil
		bluetoothLink = .idle("已断开")
	}

	/// Forget: drop secrets and records; the system bond (secure pairing) can only
	/// be removed by the user in Settings > Bluetooth.
	func forget(_ camera: PairedCamera) async {
		if bleConnectedCamera == camera.id { await disconnectBluetooth() }
		if let ssid = camera.wifiSSID { joiner.forget(ssid: ssid) }
		pairings.forget(camera.id)
	}

	// MARK: Wi-Fi

	/// Bluetooth handoff: ask the camera to start Wi-Fi, join it, open the session.
	func connectWiFiViaBluetooth() async {
		guard let camera = activeCamera, bleConnectedCamera == camera.id else {
			alert = AppAlert(title: "需要蓝牙连接", message: "请先通过蓝牙连接已配对的相机。")
			return
		}
		wifiLink = .working("正在请求相机开启 Wi-Fi…")
		do {
			let creds = try await ble.requestWiFi()
			pairings.update(camera.id) { $0.wifiSSID = creds.ssid }
			if let pw = creds.password { try? pairings.setSecret(Data(pw.utf8), .wifiPassword, for: camera.id) }
			wifiLink = .working("正在加入 \(creds.ssid)…")
			let outcome = try await joiner.join(ssid: creds.ssid, password: creds.password, hidden: creds.password != nil, cameraIP: settings.cameraIP)
			switch outcome {
			case .joined(let route):
				wifiLink = .ready("已连接 \(creds.ssid)（\(route.interfaceName)，本机 \(route.localAddress)）")
				pairings.record(camera.id, ConnectionRecord(link: .wifi, success: true, detail: creds.ssid))
				await startSession(transport: camera.preferredTransport)
			case .needsManualJoin(let reason):
				wifiLink = .failed("需要手动加入 Wi-Fi：\(reason)")
				manualJoin = ManualJoinRequest(ssid: creds.ssid, password: creds.password, reason: reason, transport: camera.preferredTransport)
			}
		} catch {
			wifiLink = .failed(error.localizedDescription)
			pairings.record(camera.id, ConnectionRecord(link: .wifi, success: false, detail: error.localizedDescription))
			if let saved = pairings.wifiPassword(for: camera.id), let ssid = camera.wifiSSID, (error as? BLEError) == nil {
				manualJoin = ManualJoinRequest(ssid: ssid, password: saved, reason: error.localizedDescription, transport: camera.preferredTransport)
			}
		}
	}

	/// Manual path: the user joined the camera network in Settings (or started
	/// Wi-Fi from the camera menu). Verify the network, then open the session.
	func beginManualWiFi(transport: CameraTransport) {
		let ssid = activeCamera?.wifiSSID
		let pw = activeCamera.flatMap { pairings.wifiPassword(for: $0.id) }
		manualJoin = ManualJoinRequest(ssid: ssid, password: pw, reason: "手动连接相机 Wi-Fi", transport: transport)
	}

	func continueAfterManualJoin(transport: CameraTransport) async {
		manualJoin = nil
		wifiLink = .working("正在确认 iPhone 已连到相机网络…")
		guard let route = await CameraNetwork.waitUntilReachable(cameraIP: settings.cameraIP, timeout: .seconds(20)) else {
			wifiLink = .failed("仍未连到相机网络：\(settings.cameraIP):55740 不可达。请确认已加入相机 Wi-Fi，且相机在等待连接。")
			return
		}
		wifiLink = .ready("已连接相机网络（\(route.interfaceName)，本机 \(route.localAddress)）")
		await startSession(transport: transport)
	}

	// MARK: PTP session

	func startSession(transport: CameraTransport) async {
		if client != nil { await endSession(reason: "重新建立会话") }
		sessionLink = .working("正在建立相机会话…")
		do {
			let client = try PTPClient(transport: transport, logger: logger)
			self.client = client
			try await client.connect(ip: settings.cameraIP)
			sessionLink = .working("等待相机确认：如相机显示连接请求，请按 OK 允许")
			try await client.setup(clientName: settings.clientName, timeout: .seconds(120))
			let state = try await client.state()
			sessionState = state
			capabilities = .from(state)
			await client.setCompressSmall(settings.compressSmall)
			ptpDeviceInfo = try? await client.deviceInfo()
			cameraID = resolveCameraID(state: state)
			if let camera = activeCamera {
				pairings.update(camera.id) {
					if let di = self.ptpDeviceInfo {
						$0.ptpModel = di.model
						$0.ptpFirmware = di.deviceVersion
						if !di.serialNumber.isEmpty { $0.ptpSerial = di.serialNumber }
					}
				}
				pairings.record(camera.id, ConnectionRecord(link: .session, success: true, detail: state.cameraState?.label ?? "camera_state=\(state.cameraStateRaw)"))
			}
			let stateLabel = state.cameraState?.label ?? "状态 \(state.cameraStateRaw)"
			sessionLink = .ready("已连接 \(state.cameraName.isEmpty ? "相机" : state.cameraName)：\(stateLabel)")
			logger.info("session", "会话建立：transport=\(state.transport.map { "\($0.rawValue)" } ?? "?") state=\(state.cameraStateRaw) remote=0x\(String(state.remoteVersion, radix: 16)) objects=\(state.numObjects)")
			queue.attach(source: client)
			if capabilities.gallery.isAvailable {
				await gallery.reload(client: client, state: state, cameraID: cameraID, imports: imports)
			}
			startEventLoop()
			Task { await refreshCameraStatus() }
		} catch {
			let text: String
			if let e = error as? PTPError, case .notOnCameraNetwork = e {
				text = e.localizedDescription
				wifiLink = .failed("未连到相机网络")
			} else {
				text = error.localizedDescription
			}
			sessionLink = .failed(text)
			client?.abort()
			client = nil
			if let camera = activeCamera {
				pairings.record(camera.id, ConnectionRecord(link: .session, success: false, detail: text))
			}
		}
	}

	/// Import identity of the camera: PTP serial if the camera reports one,
	/// otherwise the paired Bluetooth identifier, otherwise the camera name.
	private func resolveCameraID(state: CameraSessionState) -> String {
		if let serial = ptpDeviceInfo?.serialNumber, !serial.isEmpty, serial.contains(where: { $0 != "x" && $0 != "0" }) {
			logger.redactor.register(secret: serial)
			return "serial:\(serial)"
		}
		if let camera = activeCamera { return camera.cameraKey }
		return "name:\(state.cameraName)"
	}

	func endSession(reason: String?) async {
		stopEventLoop()
		if liveviewActive { await stopLiveview() }
		queue.detach(reason: reason ?? "会话已断开")
		if let client {
			await client.close(graceful: true)
		}
		client = nil
		sessionState = nil
		capabilities = .disconnected
		batteryText = nil
		storageText = nil
		gallery.reset(offlineMessage: "未连接相机")
		sessionLink = .idle(reason ?? "已断开")
	}

	private func sessionLost(_ error: Error) {
		guard let client else { return }
		logger.error("session", "连接中断：\(error.localizedDescription)")
		client.abort()
		self.client = nil
		stopEventLoop()
		liveTask?.cancel()
		liveviewActive = false
		queue.detach(reason: "连接中断：\(error.localizedDescription)")
		capabilities = .disconnected
		gallery.reset(offlineMessage: "与相机的连接已中断")
		sessionLink = .failed("连接中断：\(error.localizedDescription)")
		if case .ready = wifiLink { wifiLink = .idle("请确认 Wi-Fi 仍连接到相机") }
	}

	func disconnectAll() async {
		await endSession(reason: "已手动断开")
		await disconnectBluetooth()
		wifiLink = .idle("未连接")
	}

	private func startEventLoop() {
		eventTask?.cancel()
		eventTask = Task { [weak self] in
			while !Task.isCancelled {
				try? await Task.sleep(for: .seconds(1))
				guard let self, let client = self.client else { return }
				do {
					// Same 1 s cadence as libfuji's module.c on_idle_tick.
					try await client.pollEvents()
					let state = try await client.state()
					let countChanged = state.numObjects != self.sessionState?.numObjects
					let stateChanged = state.cameraStateRaw != self.sessionState?.cameraStateRaw
					self.sessionState = state
					if stateChanged { self.capabilities = .from(state) }
					if state.pushedHandlesPending > 0 {
						let handles = try await client.takePushedHandles()
						await self.enqueuePushed(handles, client: client)
					}
					if countChanged, self.capabilities.gallery.isAvailable, !self.liveviewActive {
						await self.gallery.reload(client: client, state: state, cameraID: self.cameraID, imports: self.imports)
					}
				} catch let e as PTPError where e.isConnectionLost {
					self.sessionLost(e)
					return
				} catch {
					self.logger.warning("session", "事件轮询失败：\(error.localizedDescription)")
				}
			}
		}
	}

	private func stopEventLoop() {
		eventTask?.cancel()
		eventTask = nil
	}

	/// Files the camera pushes in "transfer selected on camera" mode (camera_state=1).
	private func enqueuePushed(_ handles: [UInt32], client: PTPClient) async {
		var requests: [TransferRequest] = []
		for h in handles {
			guard let info = try? await client.objectInfo(handle: h, priority: .high) else { continue }
			requests.append(TransferRequest(identity: ObjectIdentity(cameraID: cameraID, info: info), lastKnownHandle: h,
											cameraDisplayName: cameraDisplayName, format: info.format, dateCreated: info.dateCreated))
		}
		if !requests.isEmpty {
			logger.info("transfer", "相机推送 \(requests.count) 个文件")
			queue.enqueue(requests, cameraReduced: settings.compressSmall && capabilities.compressSmallToggle.isAvailable)
		}
	}

	func refreshCameraStatus() async {
		guard let client else { return }
		if let d = try? await client.propertyDescriptor(0x5001) {
			batteryText = KnownProperty.named(0x5001)?.format(d.current, descriptor: d)
		} else {
			batteryText = nil
		}
		if let infos = try? await client.storageInfos(), !infos.isEmpty {
			storageText = infos.map { s in
				var parts: [String] = []
				if s.info.freeSpaceKnown { parts.append("可用 \(ByteCountFormatter.string(fromByteCount: Int64(clamping: s.info.freeSpaceBytes), countStyle: .file))") }
				if s.info.maxCapacityKnown { parts.append("共 \(ByteCountFormatter.string(fromByteCount: Int64(clamping: s.info.maxCapacity), countStyle: .file))") }
				if s.info.freeImagesKnown { parts.append("约可拍 \(s.info.freeSpaceImages) 张") }
				return parts.isEmpty ? "已报告，但未提供容量" : parts.joined(separator: "，")
			}.joined(separator: "；")
		} else {
			storageText = nil
		}
	}

	// MARK: transfers

	func download(handles: [UInt32], force: Bool = false) async {
		let requests = await gallery.requests(for: handles, cameraName: cameraDisplayName)
		guard !requests.isEmpty else { return }
		if settings.saveToPhotos, PhotoLibrarySaver.addOnlyStatus == .notDetermined {
			_ = await PhotoLibrarySaver.requestAddOnlyAccess()
		}
		let reduced = settings.compressSmall && capabilities.compressSmallToggle.isAvailable
		let added = queue.enqueue(requests, cameraReduced: reduced, force: force)
		gallery.selection.removeAll()
		updateIdleTimer()
		if added > 0 { await background.beginIfNeeded(itemCount: queue.pendingCount) }
	}

	private func transferStateChanged() {
		gallery.refreshImportStatus()
		updateIdleTimer()
	}

	private func updateIdleTimer() {
		UIApplication.shared.isIdleTimerDisabled = settings.keepScreenAwakeWhileTransferring && queue.pendingCount > 0 && queue.hasSource
	}

	func setSaveToPhotos(_ on: Bool) {
		settings.saveToPhotos = on
		queue.saveToPhotos = on
	}

	func setCompressSmall(_ on: Bool) async {
		settings.compressSmall = on
		await client?.setCompressSmall(on)
	}

	// MARK: liveview

	func startLiveview() async {
		guard let client else { return }
		guard capabilities.liveview.isAvailable else {
			alert = AppAlert(title: "无法开启实时取景", message: capabilities.liveview.reason ?? "")
			return
		}
		guard !queue.isProcessing else {
			alert = AppAlert(title: "传输进行中", message: "切换到实时取景会中断相机的图库模式，请等待当前传输完成或暂停传输后再试。")
			return
		}
		queue.isPaused = true
		liveviewError = nil
		do {
			try await client.liveviewStart()
			liveviewActive = true
			liveFrameCount = 0
			liveTask = Task { [weak self] in
				while let self, self.liveviewActive, !Task.isCancelled {
					do {
						if let frame = try await client.liveviewFrame() {
							self.liveFrame = frame
							self.liveFrameCount += 1
						}
					} catch let e as PTPError where e.isConnectionLost {
						self.sessionLost(e)
						return
					} catch {
						self.liveviewError = error.localizedDescription
						try? await Task.sleep(for: .milliseconds(300))
					}
				}
			}
		} catch {
			queue.isPaused = false
			liveviewError = error.localizedDescription
			if let e = error as? PTPError, e.isConnectionLost { sessionLost(e) }
		}
	}

	func stopLiveview() async {
		liveviewActive = false
		liveTask?.cancel()
		liveTask = nil
		liveFrame = nil
		if let client {
			do {
				try await client.liveviewStop()
			} catch let e as PTPError where e.isConnectionLost {
				sessionLost(e)
			} catch {
				liveviewError = "退出实时取景时出错：\(error.localizedDescription)"
			}
			if let state = try? await client.state(), let client = self.client {
				sessionState = state
				capabilities = .from(state)
				await gallery.reload(client: client, state: state, cameraID: cameraID, imports: imports)
			}
		}
		queue.isPaused = false
	}

	// MARK: Bluetooth camera functions

	func shutter(_ action: FujiBLE.ShutterAction) async throws {
		try await ble.shutter(action)
	}

	/// Half press, full press, release: one exposure.
	func takePicture() async throws {
		try await ble.shutter(.press)
		try await Task.sleep(for: .milliseconds(200))
		try await ble.shutter(.release)
	}

	func syncTimeNow() async {
		do {
			try await ble.syncTimeNow()
		} catch {
			alert = AppAlert(title: "时间同步失败", message: error.localizedDescription)
		}
	}

	func setTimeSync(_ on: Bool) {
		settings.timeSync = on
		ble.setTimeSyncEnabled(on)
	}

	func setLocationSync(_ on: Bool) {
		settings.locationSync = on
		if on { location.start() } else { location.stop() }
	}

	private func sendGeotagIfAllowed() {
		guard settings.locationSync else {
			logger.info("ble", "相机请求位置，但位置同步未开启，未发送")
			return
		}
		guard let fix = location.freshFix() else {
			logger.info("ble", "相机请求位置，但暂无 2 分钟内的定位结果，未发送")
			return
		}
		Task {
			do {
				try await ble.sendGeotag(latitude: fix.latitude, longitude: fix.longitude, altitudeMeters: fix.altitude, timestamp: fix.timestamp)
				lastGeotag = Date()
			} catch {
				logger.warning("ble", "发送位置失败：\(error.localizedDescription)")
			}
		}
	}

	// MARK: app lifecycle

	func scenePhaseChanged(_ phase: ScenePhase) async {
		switch phase {
		case .background:
			if liveviewActive { await stopLiveview() }
			if isScanning { stopScan() }
		case .active:
			if let client, !(await client.isConnected()) {
				sessionLost(PTPError.notConnected)
			}
			background.appDidBecomeActive()
			updateIdleTimer()
		default:
			break
		}
	}

	func setVerboseLogs(_ on: Bool) {
		settings.verboseLogs = on
		logger.verboseEnabled = on
	}

	/// Redacted diagnostic log export.
	func exportLogFile() -> URL? {
		let header = """
		CamBridge 诊断日志（已脱敏：Wi-Fi 密码、配对令牌、序列号已隐藏）
		App \(Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "?") (\(Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "?"))
		iOS \(UIDevice.current.systemVersion)
		libfuji \(CapabilityCatalog.libfujiSHA) / libpict \(CapabilityCatalog.libpictSHA)
		会话：\(sessionLink.text)
		相机状态：\(sessionState.map { "state=\($0.cameraStateRaw) transport=\($0.transport.map { "\($0.rawValue)" } ?? "?") remote=0x\(String($0.remoteVersion, radix: 16)) DF21=0x\(String($0.imageGetVersion, radix: 16)) DF22=0x\(String($0.getObjectVersion, radix: 16)) DF25=0x\(String($0.remoteImageViewVersion, radix: 16))" } ?? "无")
		"""
		let text = logger.exportText(header: header)
		let url = FileManager.default.temporaryDirectory.appendingPathComponent("CamBridge-诊断日志.txt")
		do {
			try text.write(to: url, atomically: true, encoding: .utf8)
			return url
		} catch {
			return nil
		}
	}
}
