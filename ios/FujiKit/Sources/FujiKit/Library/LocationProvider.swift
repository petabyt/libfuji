#if canImport(CoreLocation) && os(iOS)
import CoreLocation
import Foundation
import Observation

/// Foreground location for the camera's geotag requests. Permission (When In
/// Use) is requested only when the user turns location sync on.
@MainActor
@Observable
public final class LocationProvider {
	public struct Fix: Sendable, Equatable {
		public let latitude: Double
		public let longitude: Double
		public let altitude: Double
		public let horizontalAccuracy: Double
		public let timestamp: Date
	}

	public private(set) var latest: Fix?
	public private(set) var statusText = "未开启"
	public private(set) var isRunning = false

	@ObservationIgnored private let manager = CLLocationManager()
	@ObservationIgnored private var task: Task<Void, Never>?

	public init() {}

	public var authorization: CLAuthorizationStatus { manager.authorizationStatus }

	public func start() {
		guard task == nil else { return }
		if manager.authorizationStatus == .notDetermined {
			manager.requestWhenInUseAuthorization()
		}
		isRunning = true
		statusText = "正在获取位置…"
		task = Task { [weak self] in
			do {
				for try await update in CLLocationUpdate.liveUpdates() {
					guard let self else { return }
					if update.authorizationDenied || update.authorizationDeniedGlobally {
						self.statusText = "未获得定位权限（设置 > 隐私与安全性 > 定位服务）"
						continue
					}
					if let loc = update.location, loc.horizontalAccuracy >= 0 {
						self.latest = Fix(latitude: loc.coordinate.latitude, longitude: loc.coordinate.longitude,
										  altitude: loc.altitude, horizontalAccuracy: loc.horizontalAccuracy, timestamp: loc.timestamp)
						self.statusText = String(format: "精度约 %.0f 米", loc.horizontalAccuracy)
					}
				}
			} catch {
				self?.statusText = "定位失败：\(error.localizedDescription)"
			}
		}
	}

	public func stop() {
		task?.cancel()
		task = nil
		isRunning = false
		statusText = "未开启"
	}

	/// A fix no older than `maxAge`, suitable to send to the camera.
	public func freshFix(maxAge: TimeInterval = 120) -> Fix? {
		guard let f = latest, Date().timeIntervalSince(f.timestamp) <= maxAge else { return nil }
		return f
	}
}
#endif
