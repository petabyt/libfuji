import Foundation

/// Which app features are available in the current session, derived from what
/// the camera reported during fuji_setup(). Mirrors libfuji module.c's screen
/// selection (on_try_connect_wifi) plus the app's own checks.
public struct CameraCapabilities: Sendable, Equatable {
	public var gallery: Availability
	public var download: Availability
	public var liveview: Availability
	public var cameraPushedTransfer: Availability
	public var compressSmallToggle: Availability

	public enum Availability: Sendable, Equatable {
		case available
		case unavailable(String)

		public var isAvailable: Bool { self == .available }
		public var reason: String? {
			if case .unavailable(let r) = self { return r }
			return nil
		}
	}

	public static let disconnected = CameraCapabilities(
		gallery: .unavailable("未建立相机会话"),
		download: .unavailable("未建立相机会话"),
		liveview: .unavailable("未建立相机会话"),
		cameraPushedTransfer: .unavailable("未建立相机会话"),
		compressSmallToggle: .unavailable("未建立相机会话")
	)

	public static func from(_ s: CameraSessionState) -> CameraCapabilities {
		var c = CameraCapabilities.disconnected
		switch s.cameraState {
		case .remoteAccess, .remoteImageView, .remoteImageViewXApp:
			c.gallery = .available
			c.download = .available
			c.liveview = s.remoteVersion > 0 ? .available : .unavailable("相机未报告遥控版本（DF24）")
			c.cameraPushedTransfer = .unavailable("相机当前为浏览模式，未进入相机端推送模式")
		case .fullAccess:
			c.gallery = .available
			c.download = .available
			c.liveview = .unavailable("相机处于普通浏览模式（camera_state=2），libfuji 仅在遥控模式开放实时取景")
			c.cameraPushedTransfer = .unavailable("相机当前为浏览模式")
		case .multipleTransfer:
			c.gallery = .unavailable("相机处于“相机端选图传输”模式，只能接收相机推送的文件")
			c.download = .available
			c.liveview = .unavailable("相机端推送模式不支持实时取景")
			c.cameraPushedTransfer = .available
		case .pcAutoSave:
			c.gallery = .unavailable("PC AutoSave 模式未在本 App 中开放")
			c.download = .unavailable("PC AutoSave 模式未在本 App 中开放")
		case .waitForAccess, .none:
			break
		}
		if s.transport == .xappWirelessComm, c.download.isAvailable {
			c.compressSmallToggle = .available
		} else {
			c.compressSmallToggle = .unavailable("libfuji 仅在蓝牙引导的 Wi-Fi（XApp 方式）下通过 0xD226 请求缩小图片")
		}
		return c
	}
}
