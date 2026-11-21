import Foundation

/// Fujifilm Bluetooth LE protocol constants and packet codecs.
///
/// Sources (both MIT): libfuji lib/bluetooth.c @ 90aaa93 (itself derived from
/// furble) and gkoh/furble lib/furble/Fujifilm*.cpp @ 0cac22a. Two pairing
/// schemes exist and must never be mixed:
/// - legacy: write the 4-byte token from the pairing-mode advertisement to
///   91F1DE68…/ABA356EB…, then the client name to 91F1DE68…/85B9163E….
/// - secure (firmware from ~July 2025 per furble): OS-level bonding, read the
///   4-byte status from 123D8F06…/F557D96B…, write it back with byte 3 = 0x20,
///   then the client name to 123D8F06…/85B9163E….
/// Which scheme an X-T30 III uses is decided at runtime from its GATT services.
public enum FujiBLE {
	/// Bluetooth SIG company identifier in the manufacturer data (bytes d8 04).
	public static let companyID: UInt16 = 0x04D8

	public enum Service {
		/// Legacy pairing service (0x4001). Presence ⇒ legacy token scheme.
		public static let legacyPair = "91F1DE68-DFF6-466E-8B65-FF13B0F16FB8"
		/// Secure pairing service. Advertised by a bonded camera waiting to reconnect.
		public static let securePair = "123D8F06-62A1-4935-9322-833C531EE225"
		/// Advertised by a camera in secure pairing mode (furble SERVICE_UUID).
		public static let secureAdvertised = "A9D2B304-E8D6-4902-8336-352B772D7597"
		/// Advertised by legacy cameras (Camera Remote era); also the generic file service in libfuji.
		public static let cameraRemote = "AF854C2E-B214-458E-97E2-912C4ECF2CB8"
		/// Advertised by legacy cameras (XApp era).
		public static let xapp = "117C4142-EDD4-4C77-8696-DD18EEBB770A"
		public static let configuration = "4C0020FE-F3B6-40DE-ACC9-77D129067B14"
		public static let notifications = "4E941240-D01D-46B9-A5EA-67636806830B"
		public static let notification3 = "804DAA8E-FFEB-4AB3-8E75-6EDD7303208D"
		public static let shutter = "6514EB81-4E8F-458D-AA2A-E691336CDFAC"
		public static let timeSync = "E872B11F-D526-4AE1-9BB4-89A99D48FA59"
		public static let geotag = "3B46EC2B-48BA-41FD-B1B8-ED860B60D22B"
		public static let lens = "15CA59FE-620C-464D-A987-223FAB660CDE"
		public static let deviceInformation = "180A"

		/// Services the app scans for (any of them identifies a Fujifilm camera).
		public static let scanFilter = [secureAdvertised, securePair, cameraRemote, xapp, legacyPair]
	}

	public enum Characteristic {
		public static let legacyPairToken = "ABA356EB-9633-4E60-B73F-F52516DBD671"
		/// Client name. Lives under legacyPair or securePair depending on the scheme.
		public static let identity = "85B9163E-62D1-49FF-A6F5-054B4630D4A1"
		public static let secureStatus = "F557D96B-8284-4667-8793-B971C1DECA2A"
		public static let indication1 = "A68E3F66-0FCC-4395-8D4C-AA980B5877FA"
		public static let indication2 = "BD17BA04-B76B-4892-A545-B73BA1F74DAE"
		public static let notification1 = "F9150137-5D40-4801-A8DC-F7FC5B01DA50"
		public static let indication3 = "049EC406-EF75-4205-A390-08FE209C51F0"
		public static let geotagUpdate = "AD06C7B7-F41A-46F4-A29A-712055319122"
		public static let notification6 = "E6692C5C-B7CD-44F4-95FC-EDA07CE32560"
		public static let wifiSSID = "BF6DC9CF-3606-4EC9-A4C8-D77576E93EA4"
		public static let wifiPassword = "E809256A-915C-4967-92E8-53B7D4CAD213"
		public static let notification5 = "75823784-FBB7-4B71-ABAE-CD9A34072E3C"
		public static let notification7 = "AAB609C4-94DD-4D89-BC60-665D5090B828"
		public static let notification8 = "2A125640-706D-4DD1-B420-C0F4AB93C361"
		public static let notification9 = "82A9F452-C5CE-4EF5-8203-3FC9A47F8171"
		public static let notification10 = "DEEF7187-3F43-4364-9E22-11A8C8A15951"
		public static let geotagSyncInterval = "C95D91AE-B247-4D6D-8661-7DD5D6A0F85B"
		public static let notification3 = "7170FD5A-56D9-4C19-B043-7A7047D8E1A0"
		public static let shutter = "7FCF49C6-4FF0-4777-A03D-1A79166AF7A8"
		/// libfuji writes {04 00} here to make the camera start its Wi-Fi.
		public static let shutter2 = "600655E6-3637-42F1-8FB2-44EFC5C63B13"
		public static let timeSync = "B9BFD37F-CCAD-4D36-A1EE-018E792B3EDF"
		public static let geotag = "0F36EC14-29E5-411A-A1B6-64EE8383F090"
		public static let lensName = "68887A7A-10A7-4A4A-AADD-6F9DB1ABAA0E"
		public static let lensFirmware = "621A98D8-6314-4C3E-A683-EEA1D6166606"
		public static let firmwareRevision = "2A26"
		public static let modelNumber = "2A24"
		public static let manufacturerName = "2A29"
		public static let softwareRevision = "2A28"
	}

	public struct Subscription: Sendable, Hashable {
		public let service: String
		public let characteristic: String
		/// furble aborts the connection if one of these cannot be subscribed.
		public let required: Bool
	}

	/// libfuji legacy branch order; furble FujifilmBasic requires IND1/IND2/NOT1/GEOTAG/IND3.
	public static let legacySubscriptions: [Subscription] = [
		Subscription(service: Service.notifications, characteristic: Characteristic.wifiSSID, required: false),
		Subscription(service: Service.configuration, characteristic: Characteristic.indication1, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.indication2, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.notification1, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.geotagUpdate, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.indication3, required: true),
	]

	/// furble FujifilmSecure order (subscription0 required, subscription1 optional
	/// except the geotag sync interval), plus libfuji's NOT3.
	public static let secureSubscriptions: [Subscription] = [
		Subscription(service: Service.configuration, characteristic: Characteristic.indication1, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.indication2, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.notification1, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.geotagUpdate, required: true),
		Subscription(service: Service.notifications, characteristic: Characteristic.wifiSSID, required: true),
		Subscription(service: Service.notifications, characteristic: Characteristic.notification5, required: true),
		Subscription(service: Service.configuration, characteristic: Characteristic.notification6, required: false),
		Subscription(service: Service.notifications, characteristic: Characteristic.notification7, required: false),
		Subscription(service: Service.notifications, characteristic: Characteristic.notification8, required: false),
		Subscription(service: Service.notifications, characteristic: Characteristic.notification9, required: false),
		Subscription(service: Service.notifications, characteristic: Characteristic.notification10, required: false),
		Subscription(service: Service.notifications, characteristic: Characteristic.geotagSyncInterval, required: true),
		Subscription(service: Service.notification3, characteristic: Characteristic.notification3, required: false),
	]

	/// Geotag sync interval furble configures on secure cameras (seconds).
	public static let defaultGeotagSyncInterval: UInt16 = 10

	// MARK: advertisement

	public struct Advertisement: Sendable, Equatable {
		public enum Kind: Sendable, Equatable {
			/// Legacy camera in pairing mode: 4-byte token (manufacturer data type 0x02).
			case legacyPairing(token: [UInt8])
			/// Secure camera: 5-byte serial. `pairingMode` when advertising A9D2B304…,
			/// otherwise a bonded camera advertising 123D8F06… to reconnect.
			case secure(serial: [UInt8], pairingMode: Bool)
			/// Fujifilm manufacturer data in a format the sources do not describe.
			case unrecognised(length: Int, type: UInt8?)
		}

		public let kind: Kind
		public let serviceUUIDs: [String]
		public let localName: String?

		public var isPairable: Bool {
			switch kind {
			case .legacyPairing: return true
			case .secure(_, let pairingMode): return pairingMode
			case .unrecognised: return false
			}
		}

		public var schemeHint: PairingScheme? {
			switch kind {
			case .legacyPairing: return .legacyToken
			case .secure: return .secureBond
			case .unrecognised: return nil
			}
		}

		/// `manufacturerData` as delivered by CoreBluetooth: company id (LE) first.
		public static func parse(manufacturerData: Data?, serviceUUIDs: [String], localName: String?) -> Advertisement? {
			guard let m = manufacturerData, m.count >= 2 else { return nil }
			let b = [UInt8](m)
			guard UInt16(b[0]) | UInt16(b[1]) << 8 == companyID else { return nil }
			let services = serviceUUIDs.map { $0.uppercased() }
			let kind: Kind
			if b.count == 7 && b[2] == 0x02 {
				kind = .legacyPairing(token: Array(b[3..<7]))
			} else if b.count == 8 {
				kind = .secure(serial: Array(b[3..<8]), pairingMode: services.contains(Service.secureAdvertised))
			} else {
				kind = .unrecognised(length: b.count, type: b.count > 2 ? b[2] : nil)
			}
			return Advertisement(kind: kind, serviceUUIDs: services, localName: localName)
		}
	}

	// MARK: packets

	public enum ShutterAction: Sendable {
		case focus
		case press
		case release
	}

	/// Two writes to the shutter characteristic: command {01 00} then the parameter.
	public static func shutterWrites(_ action: ShutterAction) -> [Data] {
		let command = Data([0x01, 0x00])
		switch action {
		case .release: return [command, Data([0x00, 0x00])]
		case .press: return [command, Data([0x02, 0x00])]
		case .focus: return [command, Data([0x03, 0x00])]
		}
	}

	/// fujifilm_time_t: u16 year (LE), month, day, hour, minute, second.
	public static func timePacket(_ date: Date, timeZone: TimeZone) -> Data {
		var cal = Calendar(identifier: .gregorian)
		cal.timeZone = timeZone
		let c = cal.dateComponents([.year, .month, .day, .hour, .minute, .second], from: date)
		var w = ByteWriter()
		w.u16(UInt16(clamping: c.year ?? 0))
		w.u8(UInt8(clamping: c.month ?? 0))
		w.u8(UInt8(clamping: c.day ?? 0))
		w.u8(UInt8(clamping: c.hour ?? 0))
		w.u8(UInt8(clamping: c.minute ?? 0))
		w.u8(UInt8(clamping: c.second ?? 0))
		return w.data
	}

	/// geotag_t (23 bytes): i32 lat×1e7, i32 lon×1e7, i32 altitude (m), 4 pad, UTC time.
	/// Returns nil for out-of-range coordinates; the app never sends placeholder
	/// coordinates (libfuji's update_geo_data() writes a fixed 123,123 test value).
	public static func geotagPacket(latitude: Double, longitude: Double, altitudeMeters: Double, timestamp: Date) -> Data? {
		guard (-90...90).contains(latitude), (-180...180).contains(longitude), altitudeMeters.isFinite else { return nil }
		var w = ByteWriter()
		w.i32(Int32((latitude * 10_000_000).rounded()))
		w.i32(Int32((longitude * 10_000_000).rounded()))
		w.i32(Int32(clamping: Int(altitudeMeters.rounded())))
		w.raw([0, 0, 0, 0])
		w.raw([UInt8](timePacket(timestamp, timeZone: TimeZone(identifier: "UTC")!)))
		return w.data
	}

	/// Secure pairing acknowledgement: the 4 status bytes with byte 3 set to 0x20.
	public static func secureStatusAck(_ status: Data) -> Data? {
		guard status.count == 4 else { return nil }
		var b = [UInt8](status)
		b[3] = 0x20
		return Data(b)
	}

	public static func syncIntervalPacket(seconds: UInt16) -> Data {
		var w = ByteWriter()
		w.u16(seconds)
		return w.data
	}

	/// Written to `shutter2` to start the camera's Wi-Fi (libfuji fuji_bluetooth_connect_to_wifi).
	public static let wifiStartCommand = Data([0x04, 0x00])

	public enum WiFiReadiness: Sendable, Equatable {
		case ready
		case busy
		case unknown(UInt8?)
	}

	/// libfuji: IND1 byte 0 is 1 on success, 0 when the camera is busy.
	public static func wifiReadiness(fromIndication1 value: Data?) -> WiFiReadiness {
		guard let first = value?.first else { return .unknown(nil) }
		switch first {
		case 0x01: return .ready
		case 0x00: return .busy
		default: return .unknown(first)
		}
	}

	/// furble: GEOTAG_UPDATE notification {01 00} = camera asks for a location.
	public static func isGeotagRequest(_ value: Data) -> Bool {
		value.count >= 2 && value[value.startIndex] == 0x01 && value[value.startIndex + 1] == 0x00
	}

	/// Characteristic strings are NUL-padded UTF-8.
	public static func decodeString(_ value: Data?) -> String? {
		guard let value, !value.isEmpty else { return nil }
		let trimmed = value.prefix { $0 != 0 }
		let s = String(decoding: trimmed, as: UTF8.self).trimmingCharacters(in: .whitespacesAndNewlines)
		return s.isEmpty ? nil : s
	}

	/// Client name written to the identity characteristic and sent in the PTP/IP
	/// init packet: printable ASCII, at most 26 characters (PTP/IP name field).
	public static func sanitizedClientName(_ name: String) -> String {
		let ascii = name.unicodeScalars.filter { $0.value >= 0x20 && $0.value < 0x7F }
		let s = String(String.UnicodeScalarView(ascii)).trimmingCharacters(in: .whitespaces)
		return s.isEmpty ? "iPhone" : String(s.prefix(26))
	}
}

public enum PairingScheme: String, Codable, Sendable {
	case legacyToken
	case secureBond

	public var label: String {
		switch self {
		case .legacyToken: "旧版令牌配对"
		case .secureBond: "安全配对（系统绑定）"
		}
	}
}
