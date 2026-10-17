#if canImport(BackgroundTasks) && os(iOS)
import BackgroundTasks
import Foundation
import UIKit

/// Lets a user-started download batch continue for a while after the app is
/// backgrounded, using BGContinuedProcessingTask (iOS 26+). The system shows
/// progress in a Live Activity, may cancel the task at any time (the person or
/// resource pressure), and cancels it when the app is swiped away. There is no
/// unlimited background transfer: on expiry the queue is suspended, the partial
/// file is kept, and the transfer resumes (after verification) in the foreground.
///
/// Identifiers: Info.plist permits the wildcard `<bundle id>.transfer.*`. Each
/// batch uses a new concrete identifier `<bundle id>.transfer.<UUID>`, and a
/// handler is registered for exactly that identifier right before the request
/// is submitted. Registering one handler for the wildcard itself is rejected by
/// the system (Apple DTS, developer forums thread 799126), and registering the
/// same identifier twice terminates the app, hence the fresh UUID per batch.
/// Continued-processing tasks are exempt from the "register before launch
/// finishes" rule (BGTaskScheduler.h, as quoted in that thread), so nothing is
/// registered at launch.
@MainActor
public final class BackgroundTransferCoordinator {
	public static var identifierPrefix: String { (Bundle.main.bundleIdentifier ?? "app") + ".transfer" }

	private struct TaskBox: @unchecked Sendable { let task: BGContinuedProcessingTask }

	private weak var queue: TransferQueue?
	private var active: BGContinuedProcessingTask?
	/// Identifier of a request being submitted; blocks a second submission meanwhile.
	private var submitting: String?
	/// The queue was paused because the task expired in the background.
	private var suspendedOnExpiry = false
	private var monitor: Task<Void, Never>?
	private let logger: FujiLogger
	public private(set) var lastSubmitError: String?

	public init(queue: TransferQueue, logger: FujiLogger = .shared) {
		self.queue = queue
		self.logger = logger
	}

	/// Call from the user's tap that starts downloads (the system requires a
	/// person's action). Failure is not fatal: transfers continue in the foreground.
	public func beginIfNeeded(itemCount: Int) async {
		guard active == nil, submitting == nil else { return }
		let identifier = "\(Self.identifierPrefix).\(UUID().uuidString)"
		submitting = identifier
		defer { if submitting == identifier { submitting = nil } }

		let registered = BGTaskScheduler.shared.register(forTaskWithIdentifier: identifier, using: .main) { [weak self] task in
			guard let task = task as? BGContinuedProcessingTask else {
				task.setTaskCompleted(success: false)
				return
			}
			let box = TaskBox(task: task)
			MainActor.assumeIsolated {
				if let self { self.attach(box.task) } else { box.task.setTaskCompleted(success: false) }
			}
		}
		guard registered else {
			let permitted = Bundle.main.object(forInfoDictionaryKey: "BGTaskSchedulerPermittedIdentifiers") as? [String] ?? []
			lastSubmitError = "后台任务处理器注册失败"
			logger.warning("background", "后台任务处理器注册失败：\(Self.identifierPrefix).<UUID> 未被 Info.plist 允许（BGTaskSchedulerPermittedIdentifiers = \(permitted)，应包含 \(Self.identifierPrefix).*）")
			return
		}

		let request = BGContinuedProcessingTaskRequest(identifier: identifier,
													   title: "正在从相机导入",
													   subtitle: "\(itemCount) 个文件")
		request.strategy = .fail
		do {
			try await BGTaskScheduler.shared.submitTaskRequest(request)
			lastSubmitError = nil
			logger.info("background", "已提交后台继续任务（\(itemCount) 个文件）")
		} catch {
			lastSubmitError = error.localizedDescription
			logger.info("background", "未能提交后台继续任务，传输仅在前台进行：\(error.localizedDescription)")
		}
	}

	private func attach(_ task: BGContinuedProcessingTask) {
		active = task
		let box = TaskBox(task: task)
		task.expirationHandler = { [weak self] in
			Task { @MainActor in
				guard let self else { return }
				// With strategy .fail the system may end the task right after submission
				// while the app is still in front; transfers then simply keep running.
				if UIApplication.shared.applicationState != .active, let queue = self.queue, !queue.isPaused {
					queue.suspend(reason: "系统结束了后台传输，回到 App 后会校验并继续")
					self.suspendedOnExpiry = true
				}
				self.finish(box.task, success: false)
			}
		}
		monitor?.cancel()
		monitor = Task { [weak self] in
			while !Task.isCancelled {
				guard let self, let queue = self.queue else { return }
				let (done, total) = queue.aggregateProgress
				// Progress is reported in KiB so the counts fit comfortably in Int64.
				box.task.progress.totalUnitCount = Int64(max(1, total / 1024))
				box.task.progress.completedUnitCount = Int64(done / 1024)
				let remaining = queue.pendingCount
				box.task.updateTitle("正在从相机导入", subtitle: remaining > 0 ? "剩余 \(remaining) 个文件" : "即将完成")
				let waiting = queue.items.contains { $0.state == .waitingForConnection }
				if remaining == 0 || (!queue.isProcessing && waiting) {
					// Done, or the camera connection is gone: no reason to keep running.
					let failed = queue.items.contains { $0.state == .failed || $0.state == .waitingForConnection }
					self.finish(box.task, success: !failed)
					return
				}
				try? await Task.sleep(for: .milliseconds(500))
			}
		}
	}

	/// Resume a queue that was paused by a background expiry. Resumed downloads
	/// re-verify the partial file before continuing (TransferEngine).
	public func appDidBecomeActive() {
		guard suspendedOnExpiry else { return }
		suspendedOnExpiry = false
		queue?.resume()
		logger.info("background", "回到前台，继续后台被中断的传输")
	}

	private func finish(_ task: BGContinuedProcessingTask, success: Bool) {
		guard active === task else { return }
		monitor?.cancel()
		monitor = nil
		active = nil
		task.setTaskCompleted(success: success)
	}
}
#endif
