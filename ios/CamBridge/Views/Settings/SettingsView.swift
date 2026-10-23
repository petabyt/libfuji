import SwiftUI
import Photos
import CoreLocation
import FujiKit

struct SettingsView: View {
	@Environment(AppModel.self) private var model
	@Environment(\.openURL) private var openURL
	@State private var logURL: URL?
	@State private var photosStatus = PhotoLibrarySaver.addOnlyStatus

	var body: some View {
		@Bindable var settings = model.settings
		NavigationStack {
			Form {
				Section {
					TextField("客户端名称", text: $settings.clientName)
						.textInputAutocapitalization(.never)
						.autocorrectionDisabled()
					TextField("相机 IP", text: $settings.cameraIP)
						.keyboardType(.numbersAndPunctuation)
				} header: {
					Text("连接")
				} footer: {
					Text("客户端名称会显示在相机的配对/连接信息中，只能使用英文字母、数字和符号（最多 26 个字符）。相机 IP 默认 192.168.0.1（libfuji FUJI_PTP_IP）。")
				}

				Section {
					Toggle("下载后存入“照片”", isOn: Binding(get: { settings.saveToPhotos }, set: { model.setSaveToPhotos($0) }))
					Toggle("请求相机缩小图片后再传输", isOn: Binding(get: { settings.compressSmall }, set: { v in Task { await model.setCompressSmall(v) } }))
					Toggle("传输时保持屏幕常亮", isOn: $settings.keepScreenAwakeWhileTransferring)
					Toggle("记录下载前后的相机属性（诊断）", isOn: $settings.auditProperties)
				} header: {
					Text("传输")
				} footer: {
					Text("原文件始终保存在“文件”App 的 CamBridge 目录中，存入照片使用原始字节，不重新编码。“缩小图片”使用 libfuji 的 0xD226 开关，仅在蓝牙引导的 Wi-Fi 下生效，尚未经 X-T30 III 验证，缩小后的文件会单独标记，默认关闭。")
				}

				Section("权限") {
					LabeledContent("蓝牙", value: model.bluetoothAvailability.label)
					LabeledContent("照片（仅添加）", value: photosText)
					if photosStatus == .notDetermined {
						Button("请求添加照片权限") {
							Task { photosStatus = await PhotoLibrarySaver.requestAddOnlyAccess() }
						}
					}
					LabeledContent("定位", value: locationText)
					LabeledContent("本地网络", value: "首次连接相机时由系统询问")
					Button("打开本 App 的系统设置") {
						if let url = URL(string: UIApplication.openSettingsURLString) { openURL(url) }
					}
				}

				Section("已配对相机") {
					if model.pairings.cameras.isEmpty {
						Text("无").foregroundStyle(.secondary)
					}
					ForEach(model.pairings.cameras) { camera in
						NavigationLink(camera.displayName) { PairedCameraDetail(cameraID: camera.id) }
					}
				}

				Section("工具") {
					NavigationLink { USBImportView() } label: { Label("USB 导入（ImageCaptureCore）", systemImage: "cable.connector") }
					NavigationLink { RecipeViewerView() } label: { Label("配方文件查看（FP1/FP2/FP3）", systemImage: "doc.text.magnifyingglass") }
				}

				Section {
					Toggle("详细协议日志", isOn: Binding(get: { settings.verboseLogs }, set: { model.setVerboseLogs($0) }))
					NavigationLink("查看日志") { LogView() }
					Button("导出脱敏诊断日志") { logURL = model.exportLogFile() }
					if let logURL {
						ShareLink(item: logURL) { Label("分享诊断日志", systemImage: "square.and.arrow.up") }
					}
				} header: {
					Text("诊断")
				} footer: {
					Text("日志在写入前会隐藏 Wi-Fi 密码、配对令牌、相机序列号等凭据。")
				}

				Section("关于") {
					NavigationLink("能力矩阵") { CapabilityMatrixView() }
					NavigationLink("开源许可") { LicensesView() }
					LabeledContent("libfuji", value: String(CapabilityCatalog.libfujiSHA.prefix(12)))
					LabeledContent("libpict", value: String(CapabilityCatalog.libpictSHA.prefix(12)))
				}
			}
			.navigationTitle("设置")
			.onAppear { photosStatus = PhotoLibrarySaver.addOnlyStatus }
		}
	}

	private var photosText: String {
		switch photosStatus {
		case .authorized: "已允许"
		case .limited: "有限访问"
		case .denied: "已拒绝（照片仍保存在“文件”）"
		case .restricted: "受限制"
		case .notDetermined: "未请求"
		@unknown default: "未知"
		}
	}

	private var locationText: String {
		guard model.settings.locationSync else { return "未使用（未开启位置同步）" }
		switch model.location.authorization {
		case .authorizedWhenInUse, .authorizedAlways: return "已允许"
		case .denied: return "已拒绝"
		case .restricted: return "受限制"
		case .notDetermined: return "未请求"
		@unknown default: return "未知"
		}
	}
}

struct LogView: View {
	@Environment(AppModel.self) private var model
	@State private var minimum: LogLevel = .info
	@State private var entries: [LogEntry] = []

	var body: some View {
		List(entries.reversed()) { e in
			VStack(alignment: .leading, spacing: 2) {
				Text("\(e.date.formatted(date: .omitted, time: .standard)) [\(e.category)]")
					.font(.caption2).foregroundStyle(.secondary)
				Text(e.message).font(.caption.monospaced())
					.foregroundStyle(e.level >= .warning ? .orange : .primary)
			}
		}
		.navigationTitle("日志")
		.toolbar {
			Picker("级别", selection: $minimum) {
				Text("详细").tag(LogLevel.verbose)
				Text("信息").tag(LogLevel.info)
				Text("警告").tag(LogLevel.warning)
			}
		}
		.task(id: minimum) { entries = model.logger.entries(minimum: minimum) }
		.refreshable { entries = model.logger.entries(minimum: minimum) }
	}
}

struct CapabilityMatrixView: View {
	var body: some View {
		List {
			Section {
				Text("“vcam 仿真验证通过”表示在开源仿真相机（X-S10 / GFX100 II 模型）上通过，不代表 X-T30 III 真机验证。目前没有任何功能完成 X-T30 III 真机验证。")
					.font(.footnote)
			}
			ForEach(Array(Dictionary(grouping: CapabilityCatalog.entries, by: \.group).keys.sorted()), id: \.self) { group in
				Section(group) {
					ForEach(CapabilityCatalog.entries.filter { $0.group == group }) { e in
						DisclosureGroup {
							KeyValueRow(key: "源码/函数", value: e.source)
							KeyValueRow(key: "连接模式", value: e.connection)
							KeyValueRow(key: "X-T30 III", value: e.camera)
							KeyValueRow(key: "iOS", value: e.ios)
							KeyValueRow(key: "证据", value: e.evidence)
						} label: {
							VStack(alignment: .leading, spacing: 2) {
								Text(e.feature)
								Text(e.statuses.map(\.rawValue).joined(separator: " · ") + " · " + e.app.rawValue)
									.font(.caption).foregroundStyle(.secondary)
							}
						}
					}
				}
			}
		}
		.navigationTitle("能力矩阵")
	}
}

struct LicensesView: View {
	private let notices: [(String, String)] = [
		("libfuji（MIT）", "libfuji-LICENSE"),
		("libpict（Apache-2.0）", "libpict-LICENSE"),
		("ezxml（MIT）", "ezxml-LICENSE"),
		("furble（MIT，蓝牙协议参考）", "furble-LICENSE"),
	]

	var body: some View {
		List(notices, id: \.0) { notice in
			NavigationLink(notice.0) {
				ScrollView {
					Text(Self.load(notice.1)).font(.caption.monospaced()).padding()
				}
				.navigationTitle(notice.0)
			}
		}
		.navigationTitle("开源许可")
	}

	static func load(_ name: String) -> String {
		guard let url = Bundle.main.url(forResource: name, withExtension: "txt"),
			  let text = try? String(contentsOf: url, encoding: .utf8) else { return "未找到许可文本" }
		return text
	}
}
