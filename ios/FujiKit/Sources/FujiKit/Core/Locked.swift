import Synchronization

/// Reference wrapper around `Mutex` so protected state can be shared with
/// escaping closures and threads (a `Mutex` itself is non-copyable).
public final class Locked<Value: Sendable>: Sendable {
	private let mutex: Mutex<Value>

	public init(_ value: Value) {
		mutex = Mutex(value)
	}

	@discardableResult
	public func withLock<R: Sendable>(_ body: (inout sending Value) throws -> sending R) rethrows -> R {
		try mutex.withLock(body)
	}

	public var value: Value {
		mutex.withLock { $0 }
	}

	public func set(_ newValue: Value) {
		mutex.withLock { $0 = newValue }
	}
}
