#if canImport(CoreBluetooth) && os(iOS)
import CoreBluetooth
import Foundation

/// Bluetooth state as shown in the UI.
public enum BluetoothAvailability: Sendable, Equatable {
	case unknown
	case poweredOff
	case unauthorized
	case unsupported
	case resetting
	case ready

	public var label: String {
		switch self {
		case .unknown: "正在检查蓝牙"
		case .poweredOff: "蓝牙已关闭"
		case .unauthorized: "未获得蓝牙权限（设置 > 隐私与安全性 > 蓝牙）"
		case .unsupported: "此设备不支持低功耗蓝牙"
		case .resetting: "蓝牙正在重置"
		case .ready: "蓝牙可用"
		}
	}
}

public struct DiscoveredCamera: Sendable, Identifiable, Equatable {
	/// CBPeripheral.identifier (iOS does not expose the MAC address).
	public let id: UUID
	public let name: String
	public let rssi: Int
	public let advertisement: FujiBLE.Advertisement
	public let lastSeen: Date
}

public struct BLEDeviceInformation: Sendable, Equatable, Codable {
	public var model: String?
	public var firmware: String?
	public var manufacturer: String?
	public var software: String?
	public var lensName: String?
	public var lensFirmware: String?
}

public struct PairingResult: Sendable, Equatable {
	public let peripheralID: UUID
	public let name: String
	public let scheme: PairingScheme
	public let legacyToken: [UInt8]?
	public let secureSerial: [UInt8]?
	public let device: BLEDeviceInformation
	/// Optional subscriptions that the camera rejected (diagnostics).
	public let skippedSubscriptions: [String]
}

public struct CameraWiFiCredentials: Sendable, Equatable {
	public let ssid: String
	public let password: String?
	public let readiness: FujiBLE.WiFiReadiness
}

public enum BLEEvent: Sendable {
	case availability(BluetoothAvailability)
	case discovered(DiscoveredCamera)
	case connected(UUID)
	case disconnected(UUID, String?)
	case notification(characteristic: String, value: Data)
	case geotagRequested
	case timeSynced(Date)
}

public enum BLEError: Error, LocalizedError, Sendable, Equatable {
	case bluetoothUnavailable(String)
	case peripheralUnknown
	case connectFailed(String)
	case timeout(String)
	case disconnected
	case noFujifilmPairingService
	case schemeMismatch(saved: PairingScheme, found: PairingScheme)
	case notInPairingMode
	case tokenRejected(String)
	case pairingCancelledOrFailed(String)
	case peerRemovedPairing
	case unexpectedStatus(Int)
	case missingCharacteristic(String)
	case subscriptionFailed(String)
	case cameraBusy
	case wifiInfoUnavailable(String)
	case operationFailed(String)

	public var errorDescription: String? {
		switch self {
		case .bluetoothUnavailable(let s): s
		case .peripheralUnknown: "找不到该相机，请先扫描或让相机进入蓝牙广播状态"
		case .connectFailed(let s): "蓝牙连接失败：\(s)"
		case .timeout(let what): "蓝牙操作超时：\(what)"
		case .disconnected: "蓝牙连接已断开"
		case .noFujifilmPairingService: "连接的设备没有富士配对服务（91F1DE68 或 123D8F06），无法配对"
		case .schemeMismatch(let saved, let found):
			"相机当前的配对方式（\(found.label)）与保存的记录（\(saved.label)）不一致。请忘记此相机后重新配对。"
		case .notInPairingMode: "相机未处于配对模式：广播中没有配对令牌。请在相机的蓝牙设置中选择“配对登记”后重试。"
		case .tokenRejected(let s): "相机拒绝了配对令牌（\(s)）。请确认相机正处于配对登记界面，或在相机“选择配对目标”中选择本机。"
		case .pairingCancelledOrFailed(let s): "系统蓝牙配对未完成（\(s)）。请在相机上确认，并在 iPhone 弹出的配对请求中点按“配对”。"
		case .peerRemovedPairing: "相机已删除与本机的配对。请到 iPhone 设置 > 蓝牙 中对该相机“忽略此设备”，然后在 App 中重新配对。"
		case .unexpectedStatus(let n): "相机返回的配对状态长度异常（\(n) 字节）"
		case .missingCharacteristic(let c): "相机缺少所需的蓝牙特征 \(c)"
		case .subscriptionFailed(let c): "订阅相机通知失败（\(c)）"
		case .cameraBusy: "相机忙，暂时无法开启 Wi-Fi（例如正在拍摄或菜单打开）"
		case .wifiInfoUnavailable(let s): "无法从相机读取 Wi-Fi 信息：\(s)"
		case .operationFailed(let s): s
		}
	}
}

/// CoreBluetooth implementation of the Fujifilm BLE protocol (see FujiBLE).
///
/// Concurrency: every stored property is touched only on `queue`, which is also
/// the CBCentralManager delegate queue. Public async methods hop onto the queue
/// to start an operation and are resumed from the delegate callbacks.
public final class BLECameraLink: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate, @unchecked Sendable {
	public let events: AsyncStream<BLEEvent>
	private let eventSink: AsyncStream<BLEEvent>.Continuation
	private let queue = DispatchQueue(label: "camera.ble")
	private let logger: FujiLogger

	private var central: CBCentralManager!
	private var known: [UUID: CBPeripheral] = [:]
	private var peripheral: CBPeripheral?
	private var availability: BluetoothAvailability = .unknown

	// Pending operations, keyed by "op:uuid"
	private var pendingConnect: (id: UUID, cont: CheckedContinuation<Void, Error>)?
	private var pendingServices: CheckedContinuation<Void, Error>?
	private var servicesLeft = 0
	private var pendingReads: [String: CheckedContinuation<Data?, Error>] = [:]
	private var pendingWrites: [String: CheckedContinuation<Void, Error>] = [:]
	private var pendingNotify: [String: CheckedContinuation<Void, Error>] = [:]
	private var valueWaiters: [String: CheckedContinuation<Data, Error>] = [:]

	/// Write the phone's time when the camera sends NOT1 (libfuji device_callback).
	private var timeSyncEnabled = true

	public init(logger: FujiLogger = .shared) {
		self.logger = logger
		var sink: AsyncStream<BLEEvent>.Continuation!
		self.events = AsyncStream(bufferingPolicy: .bufferingNewest(256)) { sink = $0 }
		self.eventSink = sink
		super.init()
		queue.sync {
			central = CBCentralManager(delegate: self, queue: queue, options: [CBCentralManagerOptionShowPowerAlertKey: true])
		}
	}

	public func setTimeSyncEnabled(_ on: Bool) {
		queue.async { self.timeSyncEnabled = on }
	}

	// MARK: scanning

	/// Scans in the foreground for any advertiser carrying Fujifilm manufacturer data.
	public func startScan() {
		queue.async {
			guard self.central.state == .poweredOn else { return }
			// nil services: some cameras keep the service UUID in the scan response only;
			// results are filtered by the Fujifilm company id instead.
			self.central.scanForPeripherals(withServices: nil, options: [CBCentralManagerScanOptionAllowDuplicatesKey: true])
		}
	}

	public func stopScan() {
		queue.async { if self.central.state == .poweredOn { self.central.stopScan() } }
	}

	// MARK: CBCentralManagerDelegate

	public func centralManagerDidUpdateState(_ central: CBCentralManager) {
		let a: BluetoothAvailability
		switch central.state {
		case .poweredOn: a = .ready
		case .poweredOff: a = .poweredOff
		case .unauthorized: a = .unauthorized
		case .unsupported: a = .unsupported
		case .resetting: a = .resetting
		default: a = .unknown
		}
		availability = a
		eventSink.yield(.availability(a))
		if a != .ready { failAll(BLEError.bluetoothUnavailable(a.label)) }
	}

	public func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
							   advertisementData: [String: Any], rssi RSSI: NSNumber) {
		let mfg = advertisementData[CBAdvertisementDataManufacturerDataKey] as? Data
		var services = (advertisementData[CBAdvertisementDataServiceUUIDsKey] as? [CBUUID]) ?? []
		services += (advertisementData[CBAdvertisementDataOverflowServiceUUIDsKey] as? [CBUUID]) ?? []
		let localName = advertisementData[CBAdvertisementDataLocalNameKey] as? String
		guard let ad = FujiBLE.Advertisement.parse(manufacturerData: mfg, serviceUUIDs: services.map(\.uuidString), localName: localName) else {
			return
		}
		known[peripheral.identifier] = peripheral
		if case .legacyPairing(let token) = ad.kind { logger.redactor.register(secretBytes: token) }
		if case .secure(let serial, _) = ad.kind { logger.redactor.register(secretBytes: serial) }
		let camera = DiscoveredCamera(id: peripheral.identifier, name: localName ?? peripheral.name ?? "富士相机",
									  rssi: RSSI.intValue, advertisement: ad, lastSeen: Date())
		eventSink.yield(.discovered(camera))
	}

	public func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
		guard let p = pendingConnect, p.id == peripheral.identifier else { return }
		pendingConnect = nil
		p.cont.resume()
		eventSink.yield(.connected(peripheral.identifier))
	}

	public func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
		guard let p = pendingConnect, p.id == peripheral.identifier else { return }
		pendingConnect = nil
		p.cont.resume(throwing: Self.map(error, context: "连接") ?? BLEError.connectFailed("未知原因"))
	}

	public func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
		guard peripheral.identifier == self.peripheral?.identifier else { return }
		let mapped = Self.map(error, context: "断开")
		failAll(mapped ?? BLEError.disconnected)
		self.peripheral = nil
		eventSink.yield(.disconnected(peripheral.identifier, mapped?.localizedDescription))
	}

	// MARK: CBPeripheralDelegate

	public func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
		guard let cont = pendingServices else { return }
		if let error {
			pendingServices = nil
			cont.resume(throwing: Self.map(error, context: "发现服务") ?? BLEError.operationFailed(error.localizedDescription))
			return
		}
		let services = peripheral.services ?? []
		servicesLeft = services.count
		if services.isEmpty {
			pendingServices = nil
			cont.resume()
		}
		for s in services { peripheral.discoverCharacteristics(nil, for: s) }
	}

	public func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
		if let error { logger.warning("ble", "发现特征失败 \(service.uuid.uuidString)：\(error.localizedDescription)") }
		servicesLeft -= 1
		if servicesLeft <= 0, let cont = pendingServices {
			pendingServices = nil
			cont.resume()
		}
	}

	public func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
		let key = Self.key(characteristic)
		if let cont = pendingReads.removeValue(forKey: key) {
			if let error {
				cont.resume(throwing: Self.map(error, context: "读取 \(characteristic.uuid.uuidString)") ?? BLEError.operationFailed(error.localizedDescription))
			} else {
				cont.resume(returning: characteristic.value)
			}
			return
		}
		guard error == nil, let value = characteristic.value else { return }
		handleNotification(characteristic, value: value)
	}

	public func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
		guard let cont = pendingWrites.removeValue(forKey: Self.key(characteristic)) else { return }
		if let error {
			cont.resume(throwing: Self.map(error, context: "写入 \(characteristic.uuid.uuidString)") ?? BLEError.operationFailed(error.localizedDescription))
		} else {
			cont.resume()
		}
	}

	public func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
		guard let cont = pendingNotify.removeValue(forKey: Self.key(characteristic)) else { return }
		if let error {
			cont.resume(throwing: Self.map(error, context: "订阅 \(characteristic.uuid.uuidString)") ?? BLEError.subscriptionFailed(characteristic.uuid.uuidString))
		} else {
			cont.resume()
		}
	}

	private func handleNotification(_ characteristic: CBCharacteristic, value: Data) {
		let uuid = characteristic.uuid.uuidString.uppercased()
		if let waiter = valueWaiters.removeValue(forKey: Self.key(characteristic)) {
			waiter.resume(returning: value)
		}
		logger.verbose("ble", "通知 \(uuid) = \(value.hexString)")
		eventSink.yield(.notification(characteristic: uuid, value: value))
		if uuid == FujiBLE.Characteristic.notification1, timeSyncEnabled, let p = peripheral,
		   let c = Self.find(p, FujiBLE.Service.timeSync, FujiBLE.Characteristic.timeSync) {
			// libfuji: when NOT1 changes, the phone writes its time (fujifilm_time_t).
			let now = Date()
			p.writeValue(FujiBLE.timePacket(now, timeZone: .current), for: c, type: .withResponse)
			eventSink.yield(.timeSynced(now))
		}
		if uuid == FujiBLE.Characteristic.geotagUpdate, FujiBLE.isGeotagRequest(value) {
			eventSink.yield(.geotagRequested)
		}
	}

	// MARK: primitives (all state access on `queue`)

	private static func key(_ c: CBCharacteristic) -> String {
		(c.service?.uuid.uuidString.uppercased() ?? "?") + "/" + c.uuid.uuidString.uppercased()
	}

	private static func key(_ service: String, _ characteristic: String) -> String {
		CBUUID(string: service).uuidString.uppercased() + "/" + CBUUID(string: characteristic).uuidString.uppercased()
	}

	private static func find(_ p: CBPeripheral, _ service: String, _ characteristic: String) -> CBCharacteristic? {
		let s = CBUUID(string: service), c = CBUUID(string: characteristic)
		return p.services?.first { $0.uuid == s }?.characteristics?.first { $0.uuid == c }
	}

	private func failAll(_ error: Error) {
		if let p = pendingConnect { pendingConnect = nil; p.cont.resume(throwing: error) }
		if let s = pendingServices { pendingServices = nil; s.resume(throwing: error) }
		for (_, c) in pendingReads { c.resume(throwing: error) }
		for (_, c) in pendingWrites { c.resume(throwing: error) }
		for (_, c) in pendingNotify { c.resume(throwing: error) }
		for (_, c) in valueWaiters { c.resume(throwing: error) }
		pendingReads.removeAll(); pendingWrites.removeAll(); pendingNotify.removeAll(); valueWaiters.removeAll()
	}

	/// Maps CoreBluetooth errors to user-facing pairing errors.
	static func map(_ error: Error?, context: String) -> BLEError? {
		guard let error else { return nil }
		let ns = error as NSError
		if ns.domain == CBErrorDomain {
			switch CBError.Code(rawValue: ns.code) {
			case .peerRemovedPairingInformation: return .peerRemovedPairing
			case .encryptionTimedOut: return .pairingCancelledOrFailed("加密超时")
			case .connectionTimeout: return .timeout(context)
			case .peripheralDisconnected, .notConnected: return .disconnected
			case .tooManyLEPairedDevices: return .pairingCancelledOrFailed("iPhone 已绑定的低功耗蓝牙设备过多")
			default: return .operationFailed("\(context)：\(ns.localizedDescription)")
			}
		}
		if ns.domain == CBATTErrorDomain {
			switch CBATTError.Code(rawValue: ns.code) {
			case .insufficientAuthentication, .insufficientEncryption, .insufficientEncryptionKeySize, .insufficientAuthorization:
				return .pairingCancelledOrFailed("\(context)需要加密/认证")
			case .writeNotPermitted, .readNotPermitted:
				return .operationFailed("\(context)：相机不允许此操作")
			default:
				return .operationFailed("\(context)：ATT 错误 \(ns.code)")
			}
		}
		return .operationFailed("\(context)：\(ns.localizedDescription)")
	}

	/// Runs `fire` on the BLE queue after `seconds`; callers check whether their
	/// operation is still pending and fail it.
	private func armTimeout(_ seconds: Double, fire: @escaping @Sendable () -> Void) {
		queue.asyncAfter(deadline: .now() + seconds, execute: fire)
	}

	private func connectPrimitive(_ id: UUID, timeout: Double) async throws {
		try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Void, Error>) in
			queue.async {
				guard self.central.state == .poweredOn else {
					cont.resume(throwing: BLEError.bluetoothUnavailable(self.availability.label)); return
				}
				guard let p = self.known[id] ?? self.central.retrievePeripherals(withIdentifiers: [id]).first else {
					cont.resume(throwing: BLEError.peripheralUnknown); return
				}
				self.known[id] = p
				if let old = self.pendingConnect { old.cont.resume(throwing: BLEError.operationFailed("被新的连接请求取代")) }
				p.delegate = self
				self.peripheral = p
				self.pendingConnect = (id, cont)
				self.central.connect(p, options: nil)
				self.armTimeout(timeout) {
					// CoreBluetooth connects never time out on their own.
					if let pc = self.pendingConnect, pc.id == id {
						self.pendingConnect = nil
						if let stale = self.known[id] { self.central.cancelPeripheralConnection(stale) }
						pc.cont.resume(throwing: BLEError.timeout("连接相机（请确认相机蓝牙已开启且在附近）"))
					}
				}
			}
		}
	}

	private func discoverPrimitive(timeout: Double) async throws -> [String] {
		try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Void, Error>) in
			queue.async {
				guard let p = self.peripheral, p.state == .connected else { cont.resume(throwing: BLEError.disconnected); return }
				self.pendingServices = cont
				p.discoverServices(nil)
				self.armTimeout(timeout) {
					if let c = self.pendingServices { self.pendingServices = nil; c.resume(throwing: BLEError.timeout("发现蓝牙服务")) }
				}
			}
		}
		return await withCheckedContinuation { (cont: CheckedContinuation<[String], Never>) in
			queue.async { cont.resume(returning: (self.peripheral?.services ?? []).map { $0.uuid.uuidString.uppercased() }) }
		}
	}

	private func readPrimitive(_ service: String, _ characteristic: String, timeout: Double) async throws -> Data? {
		try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Data?, Error>) in
			queue.async {
				guard let p = self.peripheral, p.state == .connected else { cont.resume(throwing: BLEError.disconnected); return }
				guard let c = Self.find(p, service, characteristic) else { cont.resume(throwing: BLEError.missingCharacteristic(characteristic)); return }
				let k = Self.key(c)
				if let old = self.pendingReads.removeValue(forKey: k) { old.resume(throwing: BLEError.operationFailed("重复读取")) }
				self.pendingReads[k] = cont
				p.readValue(for: c)
				self.armTimeout(timeout) {
					if let r = self.pendingReads.removeValue(forKey: k) { r.resume(throwing: BLEError.timeout("读取 \(characteristic)")) }
				}
			}
		}
	}

	private func writePrimitive(_ service: String, _ characteristic: String, _ data: Data, timeout: Double) async throws {
		try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Void, Error>) in
			queue.async {
				guard let p = self.peripheral, p.state == .connected else { cont.resume(throwing: BLEError.disconnected); return }
				guard let c = Self.find(p, service, characteristic) else { cont.resume(throwing: BLEError.missingCharacteristic(characteristic)); return }
				let k = Self.key(c)
				if let old = self.pendingWrites.removeValue(forKey: k) { old.resume(throwing: BLEError.operationFailed("重复写入")) }
				self.pendingWrites[k] = cont
				p.writeValue(data, for: c, type: .withResponse)
				self.armTimeout(timeout) {
					if let w = self.pendingWrites.removeValue(forKey: k) { w.resume(throwing: BLEError.timeout("写入 \(characteristic)")) }
				}
			}
		}
	}

	private func notifyPrimitive(_ service: String, _ characteristic: String, timeout: Double) async throws {
		try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Void, Error>) in
			queue.async {
				guard let p = self.peripheral, p.state == .connected else { cont.resume(throwing: BLEError.disconnected); return }
				guard let c = Self.find(p, service, characteristic) else { cont.resume(throwing: BLEError.missingCharacteristic(characteristic)); return }
				if c.isNotifying { cont.resume(); return }
				let k = Self.key(c)
				self.pendingNotify[k] = cont
				// setNotifyValue writes the CCCD for notify or indicate as the characteristic requires.
				p.setNotifyValue(true, for: c)
				self.armTimeout(timeout) {
					if let n = self.pendingNotify.removeValue(forKey: k) { n.resume(throwing: BLEError.timeout("订阅 \(characteristic)")) }
				}
			}
		}
	}

	/// Waits for the next notification/indication value of a characteristic.
	private func nextValue(_ service: String, _ characteristic: String, timeout: Double) async throws -> Data {
		try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Data, Error>) in
			queue.async {
				let k = Self.key(service, characteristic)
				if let old = self.valueWaiters.removeValue(forKey: k) { old.resume(throwing: BLEError.operationFailed("重复等待")) }
				self.valueWaiters[k] = cont
				self.armTimeout(timeout) {
					if let w = self.valueWaiters.removeValue(forKey: k) { w.resume(throwing: BLEError.timeout("等待相机通知 \(characteristic)")) }
				}
			}
		}
	}

	private func connectedPeripheralName() async -> String? {
		await withCheckedContinuation { cont in queue.async { cont.resume(returning: self.peripheral?.name) } }
	}

	// MARK: pairing / connecting

	/// Connects and performs the pairing handshake of whichever scheme the camera
	/// actually exposes. Used both for first pairing and for every reconnect
	/// (furble repeats the same handshake for saved cameras).
	///
	/// - Parameters:
	///   - savedScheme: the scheme stored for this camera; a different scheme on
	///     the camera is reported, never silently switched.
	///   - legacyToken: for legacy cameras, the token from the pairing-mode
	///     advertisement (first pairing) or the stored one (reconnect).
	public func connectAndPair(peripheralID: UUID, advertisement: FujiBLE.Advertisement?, savedScheme: PairingScheme?,
							   legacyToken: [UInt8]?, secureSerial: [UInt8]?, clientName: String) async throws -> PairingResult {
		let name = FujiBLE.sanitizedClientName(clientName)
		try await connectPrimitive(peripheralID, timeout: savedScheme == nil ? 20 : 60)
		do {
			let services = Set(try await discoverPrimitive(timeout: 20))
			let hasLegacy = services.contains(FujiBLE.Service.legacyPair)
			let hasSecure = services.contains(FujiBLE.Service.securePair)
			let found: PairingScheme
			switch (hasLegacy, hasSecure) {
			case (true, false): found = .legacyToken
			case (false, true): found = .secureBond
			case (true, true): found = savedScheme ?? advertisement?.schemeHint ?? .legacyToken
			case (false, false): throw BLEError.noFujifilmPairingService
			}
			if let savedScheme, savedScheme != found {
				throw BLEError.schemeMismatch(saved: savedScheme, found: found)
			}
			logger.info("ble", "配对方式：\(found.label)（服务：legacy=\(hasLegacy) secure=\(hasSecure)）")

			var skipped: [String] = []
			var usedToken: [UInt8]?
			var serial: [UInt8]? = secureSerial
			switch found {
			case .legacyToken:
				var token = legacyToken
				if token == nil, case .legacyPairing(let t) = advertisement?.kind { token = t }
				guard let token, token.count == 4 else { throw BLEError.notInPairingMode }
				do {
					try await writePrimitive(FujiBLE.Service.legacyPair, FujiBLE.Characteristic.legacyPairToken, Data(token), timeout: 15)
				} catch let e as BLEError {
					throw BLEError.tokenRejected(e.localizedDescription)
				}
				usedToken = token
				try await writePrimitive(FujiBLE.Service.legacyPair, FujiBLE.Characteristic.identity, Data(name.utf8), timeout: 15)
				skipped = try await subscribe(FujiBLE.legacySubscriptions)
			case .secureBond:
				// Reading the status triggers iOS bonding (system pairing sheet) the first time.
				let status: Data?
				do {
					status = try await readPrimitive(FujiBLE.Service.securePair, FujiBLE.Characteristic.secureStatus, timeout: 90)
				} catch let e as BLEError where e == .timeout("读取 \(FujiBLE.Characteristic.secureStatus)") {
					throw BLEError.pairingCancelledOrFailed("等待配对确认超时")
				}
				guard let status, let ack = FujiBLE.secureStatusAck(status) else {
					throw BLEError.unexpectedStatus(status?.count ?? 0)
				}
				try await writePrimitive(FujiBLE.Service.securePair, FujiBLE.Characteristic.secureStatus, ack, timeout: 15)
				try await writePrimitive(FujiBLE.Service.securePair, FujiBLE.Characteristic.identity, Data(name.utf8), timeout: 15)
				skipped = try await subscribe(FujiBLE.secureSubscriptions)
				try await writePrimitive(FujiBLE.Service.notifications, FujiBLE.Characteristic.geotagSyncInterval,
										 FujiBLE.syncIntervalPacket(seconds: FujiBLE.defaultGeotagSyncInterval), timeout: 15)
				if serial == nil, case .secure(let s, _) = advertisement?.kind { serial = s }
			}

			// libfuji waits 100 ms before further reads ("something timing related").
			try? await Task.sleep(for: .milliseconds(100))
			let info = await readDeviceInformation(services: services)
			let pname = await connectedPeripheralName()
			return PairingResult(peripheralID: peripheralID, name: pname ?? advertisement?.localName ?? "富士相机", scheme: found,
								 legacyToken: usedToken, secureSerial: serial, device: info, skippedSubscriptions: skipped)
		} catch {
			await disconnect()
			throw error
		}
	}

	private func subscribe(_ list: [FujiBLE.Subscription]) async throws -> [String] {
		var skipped: [String] = []
		for s in list {
			do {
				try await notifyPrimitive(s.service, s.characteristic, timeout: 10)
			} catch {
				if s.required { throw BLEError.subscriptionFailed("\(s.characteristic)：\(error.localizedDescription)") }
				skipped.append(s.characteristic)
				logger.info("ble", "可选订阅失败 \(s.characteristic)：\(error.localizedDescription)")
			}
		}
		return skipped
	}

	private func readDeviceInformation(services: Set<String>) async -> BLEDeviceInformation {
		var info = BLEDeviceInformation()
		let dis = CBUUID(string: FujiBLE.Service.deviceInformation).uuidString.uppercased()
		if services.contains(dis) {
			info.model = FujiBLE.decodeString(try? await readPrimitive(FujiBLE.Service.deviceInformation, FujiBLE.Characteristic.modelNumber, timeout: 5))
			info.firmware = FujiBLE.decodeString(try? await readPrimitive(FujiBLE.Service.deviceInformation, FujiBLE.Characteristic.firmwareRevision, timeout: 5))
			info.manufacturer = FujiBLE.decodeString(try? await readPrimitive(FujiBLE.Service.deviceInformation, FujiBLE.Characteristic.manufacturerName, timeout: 5))
			info.software = FujiBLE.decodeString(try? await readPrimitive(FujiBLE.Service.deviceInformation, FujiBLE.Characteristic.softwareRevision, timeout: 5))
		}
		if services.contains(FujiBLE.Service.lens) {
			info.lensName = FujiBLE.decodeString(try? await readPrimitive(FujiBLE.Service.lens, FujiBLE.Characteristic.lensName, timeout: 5))
			info.lensFirmware = FujiBLE.decodeString(try? await readPrimitive(FujiBLE.Service.lens, FujiBLE.Characteristic.lensFirmware, timeout: 5))
		}
		return info
	}

	public func disconnect() async {
		await withCheckedContinuation { (cont: CheckedContinuation<Void, Never>) in
			queue.async {
				if let p = self.peripheral { self.central.cancelPeripheralConnection(p) }
				self.failAll(BLEError.disconnected)
				cont.resume()
			}
		}
	}

	public func isConnected() async -> Bool {
		await withCheckedContinuation { cont in queue.async { cont.resume(returning: self.peripheral?.state == .connected) } }
	}

	// MARK: camera functions

	/// libfuji fuji_bluetooth_connect_to_wifi(): read SSID, write {04 00} to
	/// start Wi-Fi, read the password, then wait for IND1 (1 = ready, 0 = busy).
	public func requestWiFi() async throws -> CameraWiFiCredentials {
		let ssidData: Data?
		do {
			ssidData = try await readPrimitive(FujiBLE.Service.notifications, FujiBLE.Characteristic.wifiSSID, timeout: 10)
		} catch {
			throw BLEError.wifiInfoUnavailable(error.localizedDescription)
		}
		guard let ssid = FujiBLE.decodeString(ssidData) else { throw BLEError.wifiInfoUnavailable("SSID 为空") }
		// Start waiting before the trigger so the IND1 response cannot be missed.
		async let indication = try? nextValue(FujiBLE.Service.configuration, FujiBLE.Characteristic.indication1, timeout: 10)
		try await writePrimitive(FujiBLE.Service.shutter, FujiBLE.Characteristic.shutter2, FujiBLE.wifiStartCommand, timeout: 10)
		let password = FujiBLE.decodeString(try? await readPrimitive(FujiBLE.Service.notifications, FujiBLE.Characteristic.wifiPassword, timeout: 10))
		if let password { logger.redactor.register(secret: password) }
		let readiness = FujiBLE.wifiReadiness(fromIndication1: await indication)
		logger.info("ble", "相机 Wi-Fi：SSID \(ssid)，密码\(password == nil ? "未提供" : "已读取")，就绪状态 \(readiness)")
		if readiness == .busy { throw BLEError.cameraBusy }
		return CameraWiFiCredentials(ssid: ssid, password: password, readiness: readiness)
	}

	/// Remote shutter over BLE (libfuji send_shutter_command / furble sendShutterCommand).
	public func shutter(_ action: FujiBLE.ShutterAction) async throws {
		for packet in FujiBLE.shutterWrites(action) {
			try await writePrimitive(FujiBLE.Service.shutter, FujiBLE.Characteristic.shutter, packet, timeout: 5)
		}
	}

	public func syncTimeNow() async throws {
		try await writePrimitive(FujiBLE.Service.timeSync, FujiBLE.Characteristic.timeSync, FujiBLE.timePacket(Date(), timeZone: .current), timeout: 5)
		eventSink.yield(.timeSynced(Date()))
	}

	/// Sends a real location fix. Callers must have the user's location permission.
	public func sendGeotag(latitude: Double, longitude: Double, altitudeMeters: Double, timestamp: Date) async throws {
		guard let packet = FujiBLE.geotagPacket(latitude: latitude, longitude: longitude, altitudeMeters: altitudeMeters, timestamp: timestamp) else {
			throw BLEError.operationFailed("位置数据无效")
		}
		try await writePrimitive(FujiBLE.Service.geotag, FujiBLE.Characteristic.geotag, packet, timeout: 5)
	}
}
#endif
