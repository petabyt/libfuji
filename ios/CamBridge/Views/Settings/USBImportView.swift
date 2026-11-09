import SwiftUI
import FujiKit

/// USB import via ImageCaptureCore. Uses the same TransferEngine (identity,
/// integrity check, SHA-256 dedup, Photos) as Wi-Fi downloads.
struct USBImportView: View {
	@Environment(AppModel.self) private var model
	@State private var browser: USBCameraBrowser?
	@State private var devices: [USBCameraBrowser.DeviceSummary] = []
	@State private var status: String?
	@State private var importing = false
	@State private var progress: (done: Int, total: Int, current: String)?
	@State private var results: [String] = []
	@State private var diagnostics: String?

	var body: some View {
		List {
			Section {
				Text("将相机的 USB 连接模式设为“USB 读卡器”，用数据线连接 iPhone。系统可能询问是否允许访问相机。")
				Button(browser == nil ? "开始查找" : "重新查找") { Task { await start() } }
				if let status { Text(status).font(.footnote).foregroundStyle(.secondary) }
			} footer: {
				Text("iOS 不允许 App 直接控制任意 USB 端点，本功能使用系统 ImageCaptureCore 读取相机文件。USB 联机拍摄、相机内 RAW 转换和设置备份/恢复未在本版开放，见“能力矩阵”。")
			}
			ForEach(devices) { d in
				Section(d.name) {
					LabeledContent("厂商/产品 ID", value: String(format: "0x%04X / 0x%04X", d.vendorID, d.productID))
					LabeledContent("富士设备", value: d.isFujifilm ? "是（VID 0x04CB）" : "否")
					LabeledContent("会话", value: d.hasOpenSession ? (d.catalogReady ? "已打开，目录已读取（\(d.fileCount) 个文件）" : "已打开，正在读取目录…") : "未打开")
					if !d.hasOpenSession {
						Button("打开") { browser?.open(d.id) }
					}
					if d.catalogReady {
						Button("导入全部新文件") { Task { await importAll(d.id) } }
							.disabled(importing)
						Button("PTP 只读诊断（GetDeviceInfo / USB 模式）") { Task { await diagnose(d.id) } }
					}
				}
			}
			if let progress {
				Section("导入") {
					ProgressView(value: Double(progress.done), total: Double(max(progress.total, 1)))
					Text("\(progress.done)/\(progress.total) \(progress.current)").font(.footnote)
				}
			}
			if !results.isEmpty {
				Section("结果") {
					ForEach(Array(results.enumerated()), id: \.offset) { _, r in Text(r).font(.footnote) }
				}
			}
			if let diagnostics {
				Section("PTP 诊断") { Text(diagnostics).font(.caption.monospaced()).textSelection(.enabled) }
			}
		}
		.navigationTitle("USB 导入")
		.onDisappear { browser?.stop() }
	}

	private func start() async {
		let b = browser ?? USBCameraBrowser(logger: model.logger)
		browser = b
		guard await b.start() else { status = "未获得访问相机内容的权限"; return }
		Task {
			for await e in b.events {
				switch e {
				case .devices(let d): devices = d
				case .status(let s): status = s
				}
			}
		}
	}

	private func importAll(_ id: String) async {
		guard let source = browser?.source(for: id) else { return }
		importing = true
		results = []
		defer { importing = false; progress = nil }
		do {
			let handles = try await source.objectHandles()
			let cameraName = devices.first { $0.id == id }?.name ?? "USB 相机"
			let cameraID = "usb:\(id)"
			for (n, h) in handles.enumerated() {
				let info = try await source.objectInfo(handle: h)
				progress = (n, handles.count, info.filename)
				let request = TransferRequest(identity: ObjectIdentity(cameraID: cameraID, info: info), lastKnownHandle: h,
											  cameraDisplayName: cameraName, format: info.format, dateCreated: info.dateCreated)
				if case .alreadyImported = model.imports.status(for: request.identity) {
					continue
				}
				do {
					let out = try await model.engine.run(request, source: source, options: TransferOptions()) { _, _ in }
					switch out.kind {
					case .imported(let record):
						var line = "✓ \(info.filename)"
						if model.settings.saveToPhotos, let url = out.fileURL {
							do {
								let asset = try await PhotoLibrarySaver().save(fileAt: url, format: record.format, originalFilename: info.filename, creationDate: info.dateCreated)
								model.imports.update(record.id) { $0.photosLocalIdentifier = asset }
							} catch {
								line += "（未存入照片：\(error.localizedDescription)）"
							}
						}
						results.append(line)
					case .duplicate:
						results.append("= \(info.filename)（内容重复，已跳过）")
					}
				} catch {
					results.append("✗ \(info.filename)：\(error.localizedDescription)")
				}
			}
			progress = (handles.count, handles.count, "完成")
		} catch {
			results.append("✗ \(error.localizedDescription)")
		}
	}

	private func diagnose(_ id: String) async {
		do {
			guard let result = try await browser?.ptpDiagnostics(for: id) else { return }
			let info = result.info
			let modeText: String
			switch result.usbMode {
			case 5?: modeText = "5（联机拍摄）"
			case 6?: modeText = "6（RAW 转换/备份恢复）"
			case 8?: modeText = "8（网络摄像头）"
			case let m?: modeText = "\(m)（未知，可能为读卡器/MTP）"
			case nil: modeText = "未提供 0xD16E"
			}
			diagnostics = """
			制造商：\(info.manufacturer)
			型号：\(info.model)
			版本：\(info.deviceVersion)
			扩展：\(info.vendorExtensionDescription)
			USB 模式（0xD16E）：\(modeText)
			支持操作 \(info.operations.count) 个，属性 \(info.properties.count) 个
			"""
		} catch {
			diagnostics = "诊断失败：\(error.localizedDescription)"
		}
	}
}
