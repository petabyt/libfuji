import Foundation

/// Constants taken from libfuji lib/fujiptp.h and libpict src/ptp.h.
/// Only values that the app actually uses are listed.
public enum FujiPTP {
	/// lib/fujiptp.h FUJI_PTP_IP / FUJI_CMD_IP_PORT
	public static let defaultCameraIP = "192.168.0.1"
	public static let commandPort: Int32 = 55740
	public static let eventPort: Int32 = 55741
	public static let liveviewPort: Int32 = 55742
	/// FUJI_MAX_PARTIAL_OBJECT in lib/fuji.h
	public static let maxPartialObject: UInt32 = 0x100000

	/// enum CameraStates (PTP_DPC_FUJI_CameraState_DF00)
	public enum CameraState: Int32, Sendable {
		case waitForAccess = 0
		case multipleTransfer = 1
		case fullAccess = 2
		case pcAutoSave = 3
		case remoteAccess = 6
		case remoteImageView = 11
		case remoteImageViewXApp = 20

		public var label: String {
			switch self {
			case .waitForAccess: "等待相机确认"
			case .multipleTransfer: "相机推送（相机端选图/传输）"
			case .fullAccess: "完全访问（图库）"
			case .pcAutoSave: "PC AutoSave"
			case .remoteAccess: "遥控模式"
			case .remoteImageView: "遥控模式-图库"
			case .remoteImageViewXApp: "XApp 图库"
			}
		}
	}

	public enum Property {
		public static let eventsList: UInt16 = 0xD212
		public static let compressSmall: UInt16 = 0xD226
		public static let enableCorrectFileSize: UInt16 = 0xD227
		public static let storageID: UInt16 = 0xD244
		public static let cameraState: UInt16 = 0xDF00
		public static let clientState: UInt16 = 0xDF01
		public static let remoteVersion: UInt16 = 0xDF24
		public static let usbMode: UInt16 = 0xD16E
		public static let batteryInfo1: UInt16 = 0xD36A
		public static let batteryInfo2: UInt16 = 0xD36B
		public static let firmwareVersion: UInt16 = 0xD153
		public static let deviceName: UInt16 = 0xD20B
	}

	public enum Operation {
		public static let getDeviceInfo: UInt16 = 0x1001
		public static let fujiGetDeviceInfo: UInt16 = 0x902B
	}

	/// Names for common PTP response codes (ISO 15740) for error messages.
	public static func responseName(_ code: UInt32) -> String {
		switch code {
		case 0x2001: "OK"
		case 0x2002: "General Error"
		case 0x2003: "Session Not Open"
		case 0x2004: "Invalid TransactionID"
		case 0x2005: "Operation Not Supported"
		case 0x2006: "Parameter Not Supported"
		case 0x2007: "Incomplete Transfer"
		case 0x2008: "Invalid StorageID"
		case 0x2009: "Invalid ObjectHandle"
		case 0x200A: "DeviceProp Not Supported"
		case 0x200C: "Store Full"
		case 0x200F: "Access Denied"
		case 0x2013: "Store Not Available"
		case 0x2019: "Device Busy"
		case 0x201C: "Invalid DeviceProp Value"
		case 0x201D: "Invalid Parameter"
		case 0x201E: "Session Already Open"
		case 0x201F: "Transaction Cancelled"
		default: String(format: "0x%04X", code)
		}
	}
}
