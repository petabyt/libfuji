#if canImport(NetworkExtension) && os(iOS)
import Foundation
import NetworkExtension

/// Joins the camera's access point with NEHotspotConfiguration (requires the
/// Hotspot Configuration entitlement) and verifies the join by reaching the
/// camera's PTP port. When the entitlement or the user's consent is missing,
/// `JoinOutcome.needsManualJoin` tells the UI to guide a manual join in Settings.
public struct HotspotJoiner: Sendable {
	public enum JoinOutcome: Sendable, Equatable {
		/// Phone is on the camera network and port 55740 accepted a TCP connection.
		case joined(CameraNetwork.Route)
		/// Automatic join impossible; show SSID/password and let the user join in Settings.
		case needsManualJoin(reason: String)
	}

	public enum JoinError: Error, LocalizedError, Sendable, Equatable {
		case userDenied
		case configurationAppliedButUnreachable(String)

		public var errorDescription: String? {
			switch self {
			case .userDenied: "你拒绝了加入相机 Wi-Fi 的请求"
			case .configurationAppliedButUnreachable(let ssid):
				"系统已接受 “\(ssid)” 的配置，但在等待时间内无法连到相机（192.168.0.1:55740）。请确认相机仍在等待连接，或在设置中手动加入该 Wi-Fi。"
			}
		}
	}

	public let logger: FujiLogger
	public init(logger: FujiLogger = .shared) { self.logger = logger }

	/// - Parameter hidden: libfuji/fudge mark the network hidden when the
	///   password came over Bluetooth; we try that first and fall back to a
	///   normal (broadcast SSID) configuration.
	public func join(ssid: String, password: String?, hidden: Bool, cameraIP: String = FujiPTP.defaultCameraIP) async throws -> JoinOutcome {
		// Already on the camera network (e.g. joined manually)?
		if let r = CameraNetwork.route(to: cameraIP), await CameraNetwork.probeTCP(host: cameraIP, port: UInt16(FujiPTP.commandPort)) {
			return .joined(r)
		}
		var attempts: [Bool] = [hidden]
		if hidden { attempts.append(false) }
		var lastFailure: Error?
		for isHidden in attempts {
			let config: NEHotspotConfiguration
			if let password, !password.isEmpty {
				config = NEHotspotConfiguration(ssid: ssid, passphrase: password, isWEP: false)
			} else {
				config = NEHotspotConfiguration(ssid: ssid)
			}
			config.hidden = isHidden
			// Keep the configuration while the app is backgrounded: joinOnce networks are
			// dropped after 15 s in the background, which would cut transfers.
			config.joinOnce = false
			do {
				try await NEHotspotConfigurationManager.shared.apply(config)
				logger.info("wifi", "已应用热点配置 \(ssid)（hidden=\(isHidden)）")
			} catch let error as NSError where error.domain == NEHotspotConfigurationErrorDomain {
				switch NEHotspotConfigurationError(rawValue: error.code) {
				case .alreadyAssociated:
					logger.info("wifi", "已连接 \(ssid)")
				case .userDenied:
					throw JoinError.userDenied
				case .applicationIsNotInForeground:
					return .needsManualJoin(reason: "App 不在前台，系统拒绝自动加入 Wi-Fi")
				case .invalidSSID, .invalidWPAPassphrase:
					return .needsManualJoin(reason: "相机提供的 Wi-Fi 信息无效（\(error.localizedDescription)）")
				case .systemDenied, .userUnauthorized:
					return .needsManualJoin(reason: "系统拒绝了自动加入：\(error.localizedDescription)")
				default:
					// .internal / .unknown are what a build without the Hotspot
					// Configuration entitlement typically gets.
					lastFailure = error
					logger.warning("wifi", "自动加入失败（\(error.code)）：\(error.localizedDescription)")
					continue
				}
			} catch {
				lastFailure = error
				continue
			}
			// Configuration accepted != joined. Verify.
			if let route = await CameraNetwork.waitUntilReachable(cameraIP: cameraIP, timeout: .seconds(25)) {
				return .joined(route)
			}
			lastFailure = JoinError.configurationAppliedButUnreachable(ssid)
		}
		if let e = lastFailure as? JoinError { throw e }
		return .needsManualJoin(reason: lastFailure.map { "无法自动加入：\($0.localizedDescription)" } ?? "无法自动加入")
	}

	/// Removes a configuration this app added (e.g. when forgetting a camera).
	public func forget(ssid: String) {
		NEHotspotConfigurationManager.shared.removeConfiguration(forSSID: ssid)
	}
}
#endif
