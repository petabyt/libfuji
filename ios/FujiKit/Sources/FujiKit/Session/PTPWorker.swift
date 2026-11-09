import Foundation

/// One dedicated OS thread per camera session. All calls into the C core for
/// that session run here, one at a time, so PTP transactions are strictly
/// ordered and libpict's per-transaction (recursive, per-thread) mutex is
/// always taken and released on the same thread.
///
/// Jobs are picked by priority (user actions before thumbnail prefetch) and
/// FIFO within a priority. A queued job whose Swift task is cancelled is
/// removed before it starts; a job that already started runs to completion
/// (long C operations are interrupted with fujiios_request_cancel/abort).
public final class PTPWorker: Sendable {
	public enum Priority: Int, Sendable, Comparable {
		case background = 0
		case normal = 1
		case high = 2
		public static func < (a: Priority, b: Priority) -> Bool { a.rawValue < b.rawValue }
	}

	private struct Job: Sendable {
		let id: UInt64
		let priority: Priority
		let work: @Sendable () -> Void
		let cancelled: @Sendable () -> Void
	}

	private struct State: Sendable {
		var jobs: [Job] = []
		var nextID: UInt64 = 0
		var stopping = false
		var running: UInt64?
	}

	private final class Shared: @unchecked Sendable {
		// `condition` guards `state` changes that the thread waits on; all state
		// access additionally goes through the Locked box.
		let condition = NSCondition()
		let state = Locked(State())
	}

	private let shared = Shared()
	public let name: String

	public init(name: String) {
		self.name = name
		let shared = self.shared
		let thread = Thread {
			PTPWorker.loop(shared)
		}
		thread.name = name
		thread.qualityOfService = .userInitiated
		thread.stackSize = 1 << 20
		thread.start()
	}

	private static func loop(_ shared: Shared) {
		while true {
			shared.condition.lock()
			var next: Job?
			var stop = false
			while true {
				let picked = shared.state.withLock { s -> (Job?, Bool) in
					guard !s.jobs.isEmpty else { return (nil, s.stopping) }
					var best = 0
					for i in s.jobs.indices.dropFirst() {
						let a = s.jobs[i], b = s.jobs[best]
						if a.priority > b.priority || (a.priority == b.priority && a.id < b.id) { best = i }
					}
					let job = s.jobs.remove(at: best)
					s.running = job.id
					return (job, false)
				}
				next = picked.0
				stop = picked.1
				if next != nil || stop { break }
				shared.condition.wait()
			}
			shared.condition.unlock()
			if let job = next {
				job.work()
				shared.state.withLock { $0.running = nil }
			} else if stop {
				return
			}
		}
	}

	private func enqueue(_ priority: Priority, work: @escaping @Sendable () -> Void, cancelled: @escaping @Sendable () -> Void) -> UInt64? {
		shared.condition.lock()
		let id = shared.state.withLock { s -> UInt64? in
			guard !s.stopping else { return nil }
			let id = s.nextID
			s.nextID &+= 1
			s.jobs.append(Job(id: id, priority: priority, work: work, cancelled: cancelled))
			return id
		}
		shared.condition.signal()
		shared.condition.unlock()
		return id
	}

	/// Removes a job that has not started yet and reports it as cancelled.
	@discardableResult
	private func remove(_ id: UInt64) -> Bool {
		let job = shared.state.withLock { s -> Job? in
			guard let idx = s.jobs.firstIndex(where: { $0.id == id }) else { return nil }
			return s.jobs.remove(at: idx)
		}
		job?.cancelled()
		return job != nil
	}

	/// Runs `body` on the worker thread and returns its result.
	public func run<T: Sendable>(_ priority: Priority = .normal, _ body: @escaping @Sendable () throws -> T) async throws -> T {
		let idBox = Locked<UInt64?>(nil)
		return try await withTaskCancellationHandler {
			try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<T, Error>) in
				let once = Locked(false)
				let resume: @Sendable (Result<T, Error>) -> Void = { result in
					let first = once.withLock { done -> Bool in
						if done { return false }
						done = true
						return true
					}
					if first { continuation.resume(with: result) }
				}
				if Task.isCancelled {
					resume(.failure(CancellationError()))
					return
				}
				guard let id = enqueue(priority, work: { resume(Result { try body() }) },
									   cancelled: { resume(.failure(CancellationError())) }) else {
					resume(.failure(PTPError.workerStopped))
					return
				}
				idBox.set(id)
				// Cancellation may have arrived between the check above and storing the id.
				if Task.isCancelled { remove(id) }
			}
		} onCancel: {
			if let id = idBox.value { remove(id) }
		}
	}

	/// Fire-and-forget job (used for teardown).
	public func submit(_ priority: Priority = .normal, _ body: @escaping @Sendable () -> Void) {
		_ = enqueue(priority, work: body, cancelled: {})
	}

	/// Number of queued (not yet started) jobs.
	public var pendingCount: Int { shared.state.withLock { $0.jobs.count } }

	/// Lets queued jobs finish, then ends the thread. Jobs submitted after stop()
	/// fail with PTPError.workerStopped.
	public func stop() {
		shared.condition.lock()
		shared.state.withLock { $0.stopping = true }
		shared.condition.broadcast()
		shared.condition.unlock()
	}

	/// Cancels every job that has not started.
	public func cancelPending() {
		let jobs = shared.state.withLock { s -> [Job] in
			let j = s.jobs
			s.jobs.removeAll()
			return j
		}
		jobs.forEach { $0.cancelled() }
	}
}
