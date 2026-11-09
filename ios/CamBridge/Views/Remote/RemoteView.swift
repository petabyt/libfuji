import SwiftUI
import UIKit
import FujiKit

struct RemoteView: View {
	@Environment(AppModel.self) private var model
	@State private var interval = IntervalShooter()
	@State private var shutterError: String?
	@State private var halfPressed = false
	@Environment(\.scenePhase) private var scenePhase

	private var bleReady: Bool { model.bleConnectedCamera != nil }

	var body: some View {
		NavigationStack {
			List {
				Section {
					if bleReady {
						shutterControls
					} else {
						UnavailableNote(title: "蓝牙遥控快门", reason: "需要先在“相机”页通过蓝牙连接已配对的相机。")
					}
					if let shutterError { Text(shutterError).font(.footnote).foregroundStyle(.orange) }
				} header: {
					Text("蓝牙遥控")
				} footer: {
					Text("快门通过蓝牙特征 7FCF49C6 发送（命令 01 00，参数 03 00 对焦 / 02 00 按下 / 00 00 释放），实际效果以相机设置为准。")
				}

				Section {
					intervalControls
				} header: {
					Text("间隔拍摄（仅前台）")
				} footer: {
					Text("由 iPhone 计时并通过蓝牙触发快门。App 进入后台时会停止，不使用后台保活。")
				}

				Section("实时取景（Wi-Fi）") {
					liveviewSection
				}

				Section("同步") {
					let settings = model.settings
					Toggle("连接后自动同步时间", isOn: Binding(get: { settings.timeSync }, set: { model.setTimeSync($0) }))
					Button("立即同步时间") { Task { await model.syncTimeNow() } }
						.disabled(!bleReady)
					if let t = model.lastTimeSync {
						Text("上次同步：\(t.formatted(date: .omitted, time: .standard))").font(.footnote).foregroundStyle(.secondary)
					}
					Toggle("向相机提供位置", isOn: Binding(get: { settings.locationSync }, set: { model.setLocationSync($0) }))
					if settings.locationSync {
						Text("定位：\(model.location.statusText)").font(.footnote).foregroundStyle(.secondary)
						if let t = model.lastGeotag {
							Text("上次发送位置：\(t.formatted(date: .omitted, time: .standard))").font(.footnote).foregroundStyle(.secondary)
						}
					}
				}

				Section {
					if model.client != nil {
						NavigationLink("拍摄参数（曝光、ISO、白平衡、胶片模拟…）") { PropertiesView() }
					} else {
						UnavailableNote(title: "拍摄参数", reason: "需要先建立 Wi-Fi 相机会话。")
					}
					UnavailableNote(title: "Wi-Fi 远程拍摄", reason: "libfuji 没有通过 PTP 触发拍摄的流程；可在取景时使用上方的蓝牙快门。")
				}
			}
			.navigationTitle("遥控")
			.onChange(of: scenePhase) { _, phase in
				if phase != .active, interval.isRunning {
					interval.stop(reason: "App 已离开前台，间隔拍摄已停止")
				}
			}
		}
	}

	@ViewBuilder private var shutterControls: some View {
		HStack(spacing: 16) {
			Button {
				Task { await act(halfPressed ? .release : .focus); halfPressed.toggle() }
			} label: {
				Label(halfPressed ? "松开对焦" : "半按对焦", systemImage: "scope")
					.frame(maxWidth: .infinity)
			}
			.buttonStyle(.bordered)
			Button {
				Task {
					do { try await model.takePicture(); shutterError = nil; halfPressed = false }
					catch { shutterError = error.localizedDescription }
				}
			} label: {
				Label("拍摄", systemImage: "camera.shutter.button.fill")
					.frame(maxWidth: .infinity)
			}
			.buttonStyle(.borderedProminent)
		}
		HoldToShootButton { pressed in
			Task { await act(pressed ? .press : .release) }
		}
	}

	private func act(_ action: FujiBLE.ShutterAction) async {
		do {
			try await model.shutter(action)
			shutterError = nil
		} catch {
			shutterError = error.localizedDescription
		}
	}

	@ViewBuilder private var intervalControls: some View {
		Stepper("间隔 \(interval.intervalSeconds) 秒", value: $interval.intervalSeconds, in: 2...3600)
			.disabled(interval.isRunning)
		Stepper("张数 \(interval.count)", value: $interval.count, in: 1...9999)
			.disabled(interval.isRunning)
		if interval.isRunning {
			Text("已拍 \(interval.taken)/\(interval.count)" + (interval.nextShot.map { "，下一张 \($0.formatted(date: .omitted, time: .standard))" } ?? ""))
				.font(.subheadline.monospacedDigit())
			Button("停止", role: .destructive) { interval.stop(reason: "已手动停止") }
		} else {
			Button("开始") {
				interval.start { try await model.takePicture() }
			}
			.disabled(!bleReady)
		}
		if let status = interval.status {
			Text(status).font(.footnote).foregroundStyle(.secondary)
		}
	}

	@ViewBuilder private var liveviewSection: some View {
		if let reason = model.capabilities.liveview.reason, !model.liveviewActive {
			UnavailableNote(title: "实时取景", reason: reason)
		} else if model.liveviewActive {
			ZStack {
				Rectangle().fill(.black)
				if let frame = model.liveFrame, let image = UIImage(data: frame) {
					Image(uiImage: image).resizable().scaledToFit()
				} else {
					ProgressView().tint(.white)
				}
			}
			.aspectRatio(3 / 2, contentMode: .fit)
			.listRowInsets(EdgeInsets())
			Text("已接收 \(model.liveFrameCount) 帧").font(.caption).foregroundStyle(.secondary)
			if bleReady {
				Button("拍摄（蓝牙快门）") { Task { try? await model.takePicture() } }
			}
			Button("退出实时取景", role: .destructive) { Task { await model.stopLiveview() } }
		} else {
			Button("开始实时取景") { Task { await model.startLiveview() } }
			Text("取景期间暂停文件传输；退出后相机回到图库模式。").font(.footnote).foregroundStyle(.secondary)
		}
		if let e = model.liveviewError { Text(e).font(.footnote).foregroundStyle(.orange) }
	}
}

/// Press-and-hold shutter (press on touch down, release on touch up) for bulb/continuous.
struct HoldToShootButton: View {
	let onChange: (Bool) -> Void
	@State private var pressed = false

	var body: some View {
		Text(pressed ? "按住中…松手释放快门" : "按住拍摄（连拍/B 门）")
			.frame(maxWidth: .infinity, minHeight: 44)
			.background(pressed ? Color.red.opacity(0.25) : Color.secondary.opacity(0.12), in: .rect(cornerRadius: 10))
			.gesture(
				DragGesture(minimumDistance: 0)
					.onChanged { _ in
						if !pressed { pressed = true; onChange(true) }
					}
					.onEnded { _ in
						pressed = false
						onChange(false)
					}
			)
			.accessibilityAddTraits(.isButton)
	}
}

/// Foreground interval shooting driven by a timer; shots go over Bluetooth.
@MainActor
@Observable
final class IntervalShooter {
	var intervalSeconds = 10
	var count = 10
	private(set) var taken = 0
	private(set) var isRunning = false
	private(set) var nextShot: Date?
	private(set) var status: String?
	@ObservationIgnored private var task: Task<Void, Never>?

	func start(shoot: @escaping @MainActor () async throws -> Void) {
		guard !isRunning else { return }
		isRunning = true
		taken = 0
		status = nil
		UIApplication.shared.isIdleTimerDisabled = true
		let interval = intervalSeconds, total = count
		task = Task { [weak self] in
			let clock = ContinuousClock()
			var deadline = clock.now
			for i in 0..<total {
				guard let self, !Task.isCancelled else { return }
				do {
					try await clock.sleep(until: deadline)
					try await shoot()
					self.taken = i + 1
				} catch is CancellationError {
					return
				} catch {
					self.status = "第 \(i + 1) 张触发失败：\(error.localizedDescription)"
				}
				deadline = deadline.advanced(by: .seconds(interval))
				self.nextShot = i + 1 < total ? Date().addingTimeInterval(TimeInterval(interval)) : nil
			}
			self?.finish(reason: "已完成")
		}
	}

	func stop(reason: String) {
		task?.cancel()
		finish(reason: reason)
	}

	private func finish(reason: String) {
		task = nil
		isRunning = false
		nextShot = nil
		status = reason
		UIApplication.shared.isIdleTimerDisabled = false
	}
}
