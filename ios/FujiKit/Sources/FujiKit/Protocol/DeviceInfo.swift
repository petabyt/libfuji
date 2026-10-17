import Foundation

/// Standard PTP DeviceInfo dataset (GetDeviceInfo, 0x1001).
public struct PTPDeviceInfo: Sendable, Hashable, Codable {
	public var standardVersion: UInt16
	public var vendorExtensionID: UInt32
	public var vendorExtensionVersion: UInt16
	public var vendorExtensionDescription: String
	public var functionalMode: UInt16
	public var operations: [UInt16]
	public var events: [UInt16]
	public var properties: [UInt16]
	public var captureFormats: [UInt16]
	public var playbackFormats: [UInt16]
	public var manufacturer: String
	public var model: String
	public var deviceVersion: String
	public var serialNumber: String

	public static func parse(_ payload: [UInt8]) throws -> PTPDeviceInfo {
		var r = ByteReader(payload)
		return PTPDeviceInfo(
			standardVersion: try r.u16(),
			vendorExtensionID: try r.u32(),
			vendorExtensionVersion: try r.u16(),
			vendorExtensionDescription: try r.ptpString(),
			functionalMode: try r.u16(),
			operations: try r.u16Array(),
			events: try r.u16Array(),
			properties: try r.u16Array(),
			captureFormats: try r.u16Array(),
			playbackFormats: try r.u16Array(),
			manufacturer: try r.ptpString(),
			model: try r.ptpString(),
			deviceVersion: try r.ptpString(),
			serialNumber: (try? r.ptpString()) ?? ""
		)
	}

	public func supports(operation: UInt16) -> Bool { operations.contains(operation) }
	public func supports(property: UInt16) -> Bool { properties.contains(property) }
}

/// Standard PTP StorageInfo dataset (GetStorageInfo, 0x1005).
public struct PTPStorageInfo: Sendable, Hashable, Codable {
	public var storageType: UInt16
	public var filesystemType: UInt16
	public var accessCapability: UInt16
	public var maxCapacity: UInt64
	public var freeSpaceBytes: UInt64
	public var freeSpaceImages: UInt32
	public var storageDescription: String
	public var volumeLabel: String

	public static func parse(_ payload: [UInt8]) throws -> PTPStorageInfo {
		var r = ByteReader(payload)
		return PTPStorageInfo(
			storageType: try r.u16(),
			filesystemType: try r.u16(),
			accessCapability: try r.u16(),
			maxCapacity: try r.u64(),
			freeSpaceBytes: try r.u64(),
			freeSpaceImages: try r.u32(),
			storageDescription: (try? r.ptpString()) ?? "",
			volumeLabel: (try? r.ptpString()) ?? ""
		)
	}

	/// PTP uses 0xFFFFFFFF... for "not reported".
	public var maxCapacityKnown: Bool { maxCapacity != UInt64.max && maxCapacity != 0 }
	public var freeSpaceKnown: Bool { freeSpaceBytes != UInt64.max }
	public var freeImagesKnown: Bool { freeSpaceImages != UInt32.max }
}
