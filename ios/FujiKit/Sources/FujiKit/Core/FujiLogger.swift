import Foundation
import Synchronization
#if canImport(os)
import os
#endif

public enum LogLevel: Int, Sendable, Comparable, Codable {
	case verbose = 0
	case info = 1
	case warning = 2
	case error = 3

	public static func < (lhs: LogLevel, rhs: LogLevel) -> Bool { lhs.rawValue < rhs.rawValue }

	var tag: String {
		switch self {
		case .verbose: "V"
		case .info: "I"
		case .warning: "W"
		case .error: "E"
		}
	}
}

public struct LogEntry: Sendable, Identifiable {
	public let id: UInt64
	public let date: Date
	public let level: LogLevel
	public let category: String
	public let message: String
}

/// Thread-safe diagnostic log with a bounded ring buffer.
///
/// Every message passes through `Redactor` before it is stored, so registered
/// secrets (Wi-Fi passwords, pairing tokens) never reach the log or an export.
/// Verbose libpict packet logs are kept only when `verboseEnabled` is on.
public final class FujiLogger: Sendable {
	public static let shared = FujiLogger()

	private struct State {
		var entries: [LogEntry] = []
		var nextID: UInt64 = 0
		var verboseEnabled = false
	}

	private let state = Mutex(State())
	private let capacity: Int
	public let redactor = Redactor()
	#if canImport(os)
	private let osLog = Logger(subsystem: "CamBridge", category: "camera")
	#endif

	public init(capacity: Int = 4000) {
		self.capacity = capacity
	}

	public var verboseEnabled: Bool {
		get { state.withLock { $0.verboseEnabled } }
		set { state.withLock { $0.verboseEnabled = newValue } }
	}

	public func log(_ level: LogLevel, _ category: String, _ message: String) {
		// Drop verbose packet logs before paying for redaction.
		if level == .verbose && !state.withLock({ $0.verboseEnabled }) { return }
		let clean = redactor.redact(message)
		let keep = state.withLock { s -> Bool in
			s.entries.append(LogEntry(id: s.nextID, date: Date(), level: level, category: category, message: clean))
			s.nextID &+= 1
			if s.entries.count > capacity {
				s.entries.removeFirst(s.entries.count - capacity)
			}
			return true
		}
		#if canImport(os)
		if keep {
			// Already redacted, so it is safe to mark public for Console.app.
			osLog.log(level: level >= .error ? .error : (level == .verbose ? .debug : .info), "[\(category, privacy: .public)] \(clean, privacy: .public)")
		}
		#else
		_ = keep
		#endif
	}

	public func verbose(_ category: String, _ message: String) { log(.verbose, category, message) }
	public func info(_ category: String, _ message: String) { log(.info, category, message) }
	public func warning(_ category: String, _ message: String) { log(.warning, category, message) }
	public func error(_ category: String, _ message: String) { log(.error, category, message) }

	public func entries(minimum: LogLevel = .verbose) -> [LogEntry] {
		state.withLock { s in s.entries.filter { $0.level >= minimum } }
	}

	public func clear() {
		state.withLock { $0.entries.removeAll() }
	}

	/// Plain-text export for sharing. Contains only redacted messages.
	public func exportText(header: String) -> String {
		let formatter = ISO8601DateFormatter()
		formatter.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
		var out = header
		if !out.hasSuffix("\n") { out += "\n" }
		out += "----\n"
		for e in entries() {
			out += "\(formatter.string(from: e.date)) \(e.level.tag) [\(e.category)] \(e.message)\n"
		}
		return out
	}
}

/// Replaces registered secrets and generic credential patterns in log text.
public final class Redactor: Sendable {
	private let secrets = Mutex<Set<String>>([])

	public init() {}

	/// Registers a value that must never appear in logs. Values shorter than
	/// 3 characters are ignored to avoid destroying unrelated text.
	public func register(secret: String) {
		let trimmed = secret.trimmingCharacters(in: .whitespacesAndNewlines)
		guard trimmed.count >= 3 else { return }
		secrets.withLock { _ = $0.insert(trimmed) }
	}

	public func register(secretBytes: [UInt8]) {
		guard !secretBytes.isEmpty else { return }
		register(secret: secretBytes.map { String(format: "%02x", $0) }.joined())
		register(secret: secretBytes.map { String(format: "%02X", $0) }.joined())
	}

	public func redact(_ text: String) -> String {
		var out = text
		let current = secrets.withLock { $0 }
		// Longest first so a secret containing another is fully removed.
		for s in current.sorted(by: { $0.count > $1.count }) where out.contains(s) {
			out = out.replacingOccurrences(of: s, with: "‹已隐藏›")
		}
		for key in ["password", "passphrase", "Password", "token", "Token"] {
			out = Redactor.maskValue(after: key, in: out)
		}
		return out
	}

	/// Masks "key: value" / "key=value" occurrences.
	static func maskValue(after key: String, in text: String) -> String {
		guard text.contains(key) else { return text }
		var result = ""
		var rest = Substring(text)
		while let range = rest.range(of: key) {
			result += rest[..<range.upperBound]
			rest = rest[range.upperBound...]
			var idx = rest.startIndex
			// skip separators
			while idx < rest.endIndex, rest[idx] == ":" || rest[idx] == "=" || rest[idx] == " " || rest[idx] == "\"" {
				idx = rest.index(after: idx)
			}
			guard idx > rest.startIndex else { continue }
			result += rest[rest.startIndex..<idx]
			var end = idx
			while end < rest.endIndex, !rest[end].isWhitespace, rest[end] != "\"", rest[end] != "," {
				end = rest.index(after: end)
			}
			if end > idx { result += "‹已隐藏›" }
			rest = rest[end...]
		}
		result += rest
		return result
	}
}
