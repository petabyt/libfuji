import Foundation
import Observation

/// A connection attempt recorded for the camera's "连接记录".
public struct ConnectionRecord: Codable, Sendable, Hashable, Identifiable {
	public enum Link: String, Codable, Sendable {
		case bluetooth, wifi, session
	}
	public var id = UUID()
	public var date: Date
	public var link: Link
	public var success: Bool
	public var detail: String

	public init(date: Date = Date(), link: Link, success: Bool, detail: String) {
		self.date = date
		self.link = link
		self.success = success
		self.detail = detail
	}
}

/// A camera paired over Bluetooth. Secrets (token, serial, Wi-Fi password) are
/// stored in the SecretStore (Keychain), never in this JSON.
public struct PairedCamera: Codable, Sendable, Hashable, Identifiable {
	/// CoreBluetooth peripheral identifier. iOS does not expose the MAC address;
	/// this UUID is stable for this app on this phone only.
	public var id: UUID
	public var displayName: String
	public var scheme: PairingScheme
	public var bleModel: String?
	public var bleFirmware: String?
	public var ptpModel: String?
	public var ptpFirmware: String?
	public var ptpSerial: String?
	public var wifiSSID: String?
	public var preferredTransport: CameraTransport
	public var createdAt: Date
	public var lastConnectedAt: Date?
	public var history: [ConnectionRecord]

	public init(id: UUID, displayName: String, scheme: PairingScheme, preferredTransport: CameraTransport = .xappWirelessComm) {
		self.id = id
		self.displayName = displayName
		self.scheme = scheme
		self.preferredTransport = preferredTransport
		self.createdAt = Date()
		self.history = []
	}

	/// Stable identifier used in import records.
	public var cameraKey: String { "ble:\(id.uuidString)" }
}

@MainActor
@Observable
public final class PairingStore {
	public private(set) var cameras: [PairedCamera] = []
	public var activeCameraID: UUID?

	@ObservationIgnored private let fileURL: URL
	@ObservationIgnored private let secrets: any SecretStore
	@ObservationIgnored private let redactor: Redactor

	public init(fileURL: URL, secrets: any SecretStore, redactor: Redactor) {
		self.fileURL = fileURL
		self.secrets = secrets
		self.redactor = redactor
		struct Stored: Codable { var cameras: [PairedCamera]; var active: UUID? }
		if let stored = JSONFile.load(Stored.self, from: fileURL) {
			cameras = stored.cameras
			activeCameraID = stored.active
		}
		for c in cameras { registerSecrets(for: c.id) }
	}

	public var activeCamera: PairedCamera? { cameras.first { $0.id == activeCameraID } }

	public func camera(_ id: UUID) -> PairedCamera? { cameras.first { $0.id == id } }

	public func upsert(_ camera: PairedCamera) {
		if let i = cameras.firstIndex(where: { $0.id == camera.id }) {
			cameras[i] = camera
		} else {
			cameras.append(camera)
		}
		save()
	}

	public func update(_ id: UUID, _ change: (inout PairedCamera) -> Void) {
		guard let i = cameras.firstIndex(where: { $0.id == id }) else { return }
		change(&cameras[i])
		save()
	}

	public func record(_ id: UUID, _ record: ConnectionRecord) {
		update(id) { c in
			c.history.insert(record, at: 0)
			if c.history.count > 50 { c.history.removeLast(c.history.count - 50) }
			if record.success { c.lastConnectedAt = record.date }
		}
	}

	public func setActive(_ id: UUID?) {
		activeCameraID = id
		save()
	}

	/// Forget a camera here and delete its secrets. The user must also remove the
	/// bond in iOS Settings > Bluetooth for secure-pairing cameras; the app cannot.
	public func forget(_ id: UUID) {
		cameras.removeAll { $0.id == id }
		if activeCameraID == id { activeCameraID = nil }
		for key in SecretKey.allCases { try? secrets.set(nil, for: key.account(id)) }
		save()
	}

	// MARK: secrets

	public enum SecretKey: String, CaseIterable {
		case legacyToken, secureSerial, wifiPassword
		func account(_ id: UUID) -> String { "\(id.uuidString).\(rawValue)" }
	}

	public func secret(_ key: SecretKey, for id: UUID) -> Data? {
		secrets.get(key.account(id))
	}

	public func setSecret(_ value: Data?, _ key: SecretKey, for id: UUID) throws {
		try secrets.set(value, for: key.account(id))
		registerSecrets(for: id)
	}

	public func wifiPassword(for id: UUID) -> String? {
		secret(.wifiPassword, for: id).map { String(decoding: $0, as: UTF8.self) }
	}

	private func registerSecrets(for id: UUID) {
		if let t = secrets.get(SecretKey.legacyToken.account(id)) { redactor.register(secretBytes: [UInt8](t)) }
		if let s = secrets.get(SecretKey.secureSerial.account(id)) { redactor.register(secretBytes: [UInt8](s)) }
		if let p = secrets.get(SecretKey.wifiPassword.account(id)) { redactor.register(secret: String(decoding: p, as: UTF8.self)) }
	}

	private func save() {
		struct Stored: Codable { var cameras: [PairedCamera]; var active: UUID? }
		do {
			try JSONFile.save(Stored(cameras: cameras, active: activeCameraID), to: fileURL)
		} catch {
			FujiLogger.shared.error("store", "保存配对记录失败：\(error.localizedDescription)")
		}
	}
}
