import SwiftUI
import FujiKit

struct TransfersView: View {
	@Environment(AppModel.self) private var model
	@State private var confirmCancelAll = false
	@State private var exporting: ExportURLs?

	struct ExportURLs: Identifiable {
		let urls: [URL]
		var id: Int { urls.count }
	}

	var body: some View {
		let queue = model.queue
		NavigationStack {
			List {
				Section {
					summary
				}
				if queue.items.isEmpty {
					ContentUnavailableView("没有传输任务", systemImage: "arrow.down.circle",
										   description: Text("在“图库”中选择照片后点按“下载”。"))
				}
				ForEach(queue.items.reversed()) { item in
					TransferRow(item: item, speed: item.id == queue.activeItem?.id ? queue.currentSpeed : nil)
						.swipeActions {
							if item.state.isFinished {
								Button("移除", role: .destructive) { queue.remove(item.id) }
							} else {
								Button("取消", role: .destructive) { queue.cancel(item.id) }
							}
							if item.state == .failed || item.state == .cancelled {
								Button("重试") { queue.retry(item.id) }.tint(.blue)
							}
						}
				}
			}
			.navigationTitle("传输")
			.toolbar {
				ToolbarItem(placement: .topBarLeading) {
					Menu {
						Button(queue.isPaused ? "继续" : "暂停", systemImage: queue.isPaused ? "play" : "pause") {
							if queue.isPaused { queue.resume() } else { queue.suspend(reason: "已手动暂停") }
						}
						Button("导出已完成到“文件”…", systemImage: "folder") {
							let urls = queue.items.compactMap { $0.localRelativePath }.map { model.paths.documents.appendingPathComponent($0) }
								.filter { FileManager.default.fileExists(atPath: $0.path) }
							if !urls.isEmpty { exporting = ExportURLs(urls: urls) }
						}
						Button("清除已结束的任务", systemImage: "trash") { queue.clearFinished() }
						Button("全部取消", systemImage: "xmark.circle", role: .destructive) { confirmCancelAll = true }
					} label: {
						Label("更多", systemImage: "ellipsis.circle")
					}
				}
			}
			.confirmationDialog("取消全部 \(queue.pendingCount) 个未完成的任务？", isPresented: $confirmCancelAll, titleVisibility: .visible) {
				Button("全部取消", role: .destructive) { queue.cancelAll() }
			} message: {
				Text("已完成的文件不受影响；未完成文件的临时数据会被删除。")
			}
			.sheet(item: $exporting) { e in
				DocumentExporter(urls: e.urls) { exporting = nil }
			}
		}
	}

	@ViewBuilder private var summary: some View {
		let queue = model.queue
		let progress = queue.aggregateProgress
		VStack(alignment: .leading, spacing: 6) {
			HStack {
				Text(queue.isProcessing ? "正在传输" : (queue.isPaused ? "已暂停" : (queue.hasSource ? "空闲" : "未连接相机")))
					.font(.headline)
				Spacer()
				if queue.isProcessing { Text(queue.currentSpeed.speedText).font(.subheadline.monospacedDigit()) }
			}
			if progress.total > 0 {
				ProgressView(value: Double(progress.done), total: Double(max(progress.total, 1)))
				Text("\(progress.done.bytesText) / \(progress.total.bytesText) · 未完成 \(queue.pendingCount) 项")
					.font(.footnote).foregroundStyle(.secondary)
			}
			if queue.items.contains(where: { $0.state == .waitingForConnection }) {
				Label("部分任务在等待重新连接相机；连接后会先校验已下载部分再继续。", systemImage: "wifi.exclamationmark")
					.font(.footnote).foregroundStyle(.orange)
			}
			Text("App 进入后台后，系统可能允许传输继续一段时间（显示为实时活动），也可能随时停止；回到 App 会继续未完成的任务。")
				.font(.caption).foregroundStyle(.secondary)
		}
	}
}

struct TransferRow: View {
	let item: TransferItem
	let speed: Double?

	var body: some View {
		VStack(alignment: .leading, spacing: 4) {
			HStack {
				Text(item.request.identity.filename).font(.headline)
				if item.cameraReduced {
					Text("相机缩小版").font(.caption2).padding(.horizontal, 4).background(.yellow.opacity(0.3), in: .capsule)
				}
				Spacer()
				Text(item.state.label).font(.subheadline).foregroundStyle(color)
			}
			if item.state == .running || item.state == .waitingForConnection || item.bytesReceived > 0 && !item.state.isFinished {
				ProgressView(value: item.fractionCompleted)
			}
			HStack {
				Text("\(item.bytesReceived.bytesText) / \(item.totalBytes.bytesText)")
				if let speed, speed > 0 { Text("· \(speed.speedText)") }
				if let avg = item.averageSpeed, item.state.isFinished { Text("· 平均 \(avg.speedText)") }
				if item.attempts > 1 { Text("· 第 \(item.attempts) 次尝试") }
			}
			.font(.caption.monospacedDigit()).foregroundStyle(.secondary)
			if let error = item.lastError {
				Text(error).font(.footnote).foregroundStyle(.orange)
			}
			switch item.photosStatus {
			case .saved: Label("已存入照片", systemImage: "photo.badge.checkmark").font(.caption).foregroundStyle(.green)
			case .denied: Label("未获照片权限，已保存在“文件”", systemImage: "folder").font(.caption).foregroundStyle(.secondary)
			case .failed(let why): Label("未存入照片：\(why)", systemImage: "exclamationmark.triangle").font(.caption).foregroundStyle(.orange)
			case .notRequested: EmptyView()
			}
			if !item.notes.isEmpty {
				DisclosureGroup("详情") {
					ForEach(Array(item.notes.enumerated()), id: \.offset) { _, note in
						Text(note).font(.caption).foregroundStyle(.secondary)
					}
				}
				.font(.caption)
			}
		}
	}

	private var color: Color {
		switch item.state {
		case .completed: .green
		case .failed: .red
		case .waitingForConnection, .cancelled: .orange
		case .duplicate: .secondary
		default: .blue
		}
	}
}
