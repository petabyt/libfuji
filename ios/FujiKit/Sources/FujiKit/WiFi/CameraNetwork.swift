import Foundation
import CFujiCore
#if canImport(Glibc)
import Glibc
#elseif canImport(Darwin)
import Darwin
#endif

/// Checks that the phone is actually on the camera's network. A successful
/// NEHotspotConfiguration apply() only means the system accepted the
/// configuration; this verifies an IPv4 route and a TCP connection.
public enum CameraNetwork {
	public struct Route: Sendable, Equatable {
		public let interfaceName: String
		public let localAddress: String
	}

	/// Interface whose subnet contains the camera address (e.g. en0 with 192.168.0.x).
	public static func route(to cameraIP: String) -> Route? {
		var ifname = [CChar](repeating: 0, count: 32)
		var local = [CChar](repeating: 0, count: 64)
		guard fujiios_find_interface_for_ipv4(cameraIP, &ifname, ifname.count, &local, local.count) == 0 else { return nil }
		func str(_ a: [CChar]) -> String { String(decoding: a.prefix { $0 != 0 }.map { UInt8(bitPattern: $0) }, as: UTF8.self) }
		return Route(interfaceName: str(ifname), localAddress: str(local))
	}

	/// Non-blocking TCP connect with a timeout. Runs off the main actor.
	@concurrent
	public static func probeTCP(host: String, port: UInt16, timeout: Duration = .seconds(2)) async -> Bool {
		var addr = sockaddr_in()
		addr.sin_family = sa_family_t(AF_INET)
		addr.sin_port = port.bigEndian
		guard inet_pton(AF_INET, host, &addr.sin_addr) == 1 else { return false }
		#if canImport(Glibc)
		let fd = socket(AF_INET, Int32(SOCK_STREAM.rawValue), 0)
		#else
		let fd = socket(AF_INET, SOCK_STREAM, 0)
		#endif
		guard fd >= 0 else { return false }
		defer { close(fd) }
		#if canImport(Darwin)
		var one: Int32 = 1
		setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, socklen_t(MemoryLayout<Int32>.size))
		#endif
		let flags = fcntl(fd, F_GETFL, 0)
		_ = fcntl(fd, F_SETFL, flags | O_NONBLOCK)
		let rc = withUnsafePointer(to: &addr) {
			$0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
				connect(fd, $0, socklen_t(MemoryLayout<sockaddr_in>.size))
			}
		}
		if rc == 0 { return true }
		guard errno == EINPROGRESS else { return false }
		var pfd = pollfd(fd: fd, events: Int16(POLLOUT), revents: 0)
		let ms = Int32(timeout.components.seconds * 1000 + timeout.components.attoseconds / 1_000_000_000_000_000)
		guard poll(&pfd, 1, ms) == 1 else { return false }
		var err: Int32 = 0
		var len = socklen_t(MemoryLayout<Int32>.size)
		getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len)
		return err == 0
	}

	/// Waits until the camera address is routable and its PTP port accepts TCP.
	public static func waitUntilReachable(cameraIP: String = FujiPTP.defaultCameraIP, port: UInt16 = UInt16(FujiPTP.commandPort),
										  timeout: Duration = .seconds(30)) async -> Route? {
		let clock = ContinuousClock()
		let deadline = clock.now + timeout
		while clock.now < deadline, !Task.isCancelled {
			if let r = route(to: cameraIP), await probeTCP(host: cameraIP, port: port) {
				return r
			}
			try? await Task.sleep(for: .milliseconds(700))
		}
		return nil
	}
}
