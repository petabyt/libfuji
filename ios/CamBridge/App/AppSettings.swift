import Foundation
import Observation
import FujiKit

/// User preferences, persisted as JSON in Application Support.
@MainActor
@Observable
final class AppSettings {
	struct Stored: Codable {
		var clientName: String
		var saveToPhotos = true
		var compressSmall = false
		var auditProperties = false
		var timeSync = true
		var locationSync = false
		var verboseLogs = false
		var cameraIP = FujiPTP.defaultCameraIP
		var keepScreenAwakeWhileTransferring = true
	}

	var clientName: String { didSet { save() } }
	var saveToPhotos: Bool { didSet { save() } }
	/// libfuji compress_images_for_smartphone (0xD226 = 1). Off by default: originals.
	var compressSmall: Bool { didSet { save() } }
	var auditProperties: Bool { didSet { save() } }
	var timeSync: Bool { didSet { save() } }
	var locationSync: Bool { didSet { save() } }
	var verboseLogs: Bool { didSet { save() } }
	var cameraIP: String { didSet { save() } }
	var keepScreenAwakeWhileTransferring: Bool { didSet { save() } }

	@ObservationIgnored private let url: URL

	init(url: URL) {
		self.url = url
		let s: Stored
		if let data = try? Data(contentsOf: url), let decoded = try? JSONDecoder().decode(Stored.self, from: data) {
			s = decoded
		} else {
			// A stable, ASCII-only name the camera can display (PTP/IP name field limit).
			let suffix = String(format: "%04X", UInt16.random(in: 0...UInt16.max))
			s = Stored(clientName: "CamBridge-\(suffix)")
		}
		clientName = s.clientName
		saveToPhotos = s.saveToPhotos
		compressSmall = s.compressSmall
		auditProperties = s.auditProperties
		timeSync = s.timeSync
		locationSync = s.locationSync
		verboseLogs = s.verboseLogs
		cameraIP = s.cameraIP
		keepScreenAwakeWhileTransferring = s.keepScreenAwakeWhileTransferring
		save()
	}

	private func save() {
		let s = Stored(clientName: clientName, saveToPhotos: saveToPhotos, compressSmall: compressSmall,
					   auditProperties: auditProperties, timeSync: timeSync, locationSync: locationSync,
					   verboseLogs: verboseLogs, cameraIP: cameraIP,
					   keepScreenAwakeWhileTransferring: keepScreenAwakeWhileTransferring)
		do {
			try FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
			try JSONEncoder().encode(s).write(to: url, options: .atomic)
		} catch {
			FujiLogger.shared.error("settings", "保存设置失败：\(error.localizedDescription)")
		}
	}
}
