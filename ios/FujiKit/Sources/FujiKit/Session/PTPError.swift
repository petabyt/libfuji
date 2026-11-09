import Foundation
import CFujiCore

/// Typed mirrors of the FUJIIOS_* return codes (anonymous C enums import as Int).
public enum CoreRC {
	public static let ok = Int32(FUJIIOS_OK)
	public static let io = Int32(FUJIIOS_ERR_IO)
	public static let runtime = Int32(FUJIIOS_ERR_RUNTIME)
	public static let unsupported = Int32(FUJIIOS_ERR_UNSUPPORTED)
	public static let checkCode = Int32(FUJIIOS_ERR_CHECK_CODE)
	public static let canceled = Int32(FUJIIOS_ERR_CANCELED)
	public static let ignored = Int32(FUJIIOS_ERR_IGNORED)
	public static let outOfMemory = Int32(FUJIIOS_ERR_OUT_OF_MEM)
	public static let invalidArg = Int32(FUJIIOS_ERR_INVALID_ARG)
	public static let notConnected = Int32(FUJIIOS_ERR_NOT_CONNECTED)
	public static let aborted = Int32(FUJIIOS_ERR_ABORTED)
}

/// Errors from the camera session. `localizedDescription` is user-facing Chinese.
public enum PTPError: Error, Sendable, Equatable, LocalizedError {
	/// A C-layer return code (FUJIIOS_ERR_*), with the PTP response code if the
	/// camera rejected the request.
	case core(code: Int32, responseCode: UInt32, operation: String)
	case notConnected
	case workerStopped
	case invalidPayload(String)
	case cameraHasNoObjects
	case setupTimedOut(seconds: Int)
	case notOnCameraNetwork(cameraIP: String)

	public var coreCode: Int32? {
		if case .core(let code, _, _) = self { return code }
		return nil
	}

	/// The connection is gone and the session must be re-established.
	public var isConnectionLost: Bool {
		switch self {
		case .core(let code, _, _):
			return code == CoreRC.io || code == CoreRC.aborted || code == CoreRC.ignored || code == CoreRC.notConnected
		case .notConnected, .workerStopped:
			return true
		default:
			return false
		}
	}

	public var isCancelled: Bool { coreCode == CoreRC.canceled }

	public var errorDescription: String? {
		switch self {
		case .core(let code, let response, let op):
			let base: String
			switch code {
			case CoreRC.io, CoreRC.ignored: base = "与相机的连接中断"
			case CoreRC.aborted: base = "操作已中止"
			case CoreRC.canceled: base = "已取消"
			case CoreRC.checkCode: base = "相机拒绝了请求（\(FujiPTP.responseName(response))）"
			case CoreRC.unsupported: base = "当前连接模式不支持此操作"
			case CoreRC.outOfMemory: base = "内存不足"
			case CoreRC.runtime: base = "相机返回了无法处理的数据"
			case CoreRC.notConnected: base = "尚未连接相机"
			case CoreRC.invalidArg: base = "参数无效"
			default: base = "未知错误（\(code)）"
			}
			return op.isEmpty ? base : "\(op)：\(base)"
		case .notConnected: return "尚未连接相机"
		case .workerStopped: return "相机会话已结束"
		case .invalidPayload(let what): return "相机数据无法解析：\(what)"
		case .cameraHasNoObjects: return "相机存储卡中没有可浏览的文件"
		case .setupTimedOut(let s): return "等待相机确认超时（\(s) 秒）。请在相机上按 OK 允许连接后重试。"
		case .notOnCameraNetwork(let ip): return "iPhone 当前不在相机 Wi-Fi 网络中（找不到通往 \(ip) 的本地接口）"
		}
	}

	/// Diagnostic form for logs (no secrets involved).
	public var debugName: String {
		switch self {
		case .core(let code, let response, let op):
			return "\(op) rc=\(String(cString: fujiios_error_name(code)))" + (response != 0 ? String(format: " resp=0x%04X", response) : "")
		default:
			return String(describing: self)
		}
	}
}
