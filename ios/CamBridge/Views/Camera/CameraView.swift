import SwiftUI
import FujiKit

struct CameraView: View {
	@Environment(AppModel.self) private var model
	@State private var showPairing = false
	@State private var busy = false

	var body: some View {
		NavigationStack {
			List {
				Section("连接状态") {
					LinkStatusRow(title: "蓝牙", systemImage: "dot.radiowaves.left.and.right", status: model.bluetoothLink)
					LinkStatusRow(title: "Wi-Fi", systemImage: "wifi", status: model.wifiLink)
					LinkStatusRow(title: "相机会话（PTP/IP）", systemImage: "camera.badge.ellipsis", status: model.sessionLink)
				}

				if model.bluetoothAvailability != .ready && model.bluetoothAvailability != .unknown {
					Section {
						Label(model.bluetoothAvailability.label, systemImage: "exclamationmark.triangle")
							.foregroundStyle(.orange)
					}
				}

				activeCameraSection
				actionsSection
				if model.sessionState != nil { cameraInfoSection }

				Section("其他相机") {
					ForEach(model.pairings.cameras.filter { $0.id != model.activeCamera?.id }) { camera in
						Button {
							model.pairings.setActive(camera.id)
						} label: {
							Label(camera.displayName, systemImage: "camera")
						}
					}
					Button {
						showPairing = true
					} label: {
						Label("添加相机（蓝牙配对）", systemImage: "plus.circle")
					}
				}
			}
			.navigationTitle("相机")
			.sheet(isPresented: $showPairing) {
				PairingSheet()
			}
		}
	}

	@ViewBuilder private var activeCameraSection: some View {
		if let camera = model.activeCamera {
			Section("当前相机") {
				NavigationLink {
					PairedCameraDetail(cameraID: camera.id)
				} label: {
					VStack(alignment: .leading, spacing: 4) {
						Text(camera.displayName).font(.headline)
						Text("\(camera.scheme.label) · \(camera.preferredTransport.label)")
							.font(.footnote).foregroundStyle(.secondary)
						if let last = camera.lastConnectedAt {
							Text("上次连接：\(last.formatted(date: .abbreviated, time: .shortened))")
								.font(.footnote).foregroundStyle(.secondary)
						}
					}
				}
			}
		} else {
			Section {
				ContentUnavailableView {
					Label("尚未配对相机", systemImage: "camera")
				} description: {
					Text("在相机的蓝牙/智能手机连接设置中开始配对登记（菜单名以相机实际显示为准），然后点按下方按钮扫描。")
				} actions: {
					Button("扫描并配对") { showPairing = true }
						.buttonStyle(.borderedProminent)
				}
			}
		}
	}

	@ViewBuilder private var actionsSection: some View {
		Section("操作") {
			if let camera = model.activeCamera {
				if model.bleConnectedCamera == camera.id {
					Button {
						run { await model.connectWiFiViaBluetooth() }
					} label: {
						Label("通过蓝牙开启相机 Wi-Fi 并连接", systemImage: "wifi.router")
					}
					.disabled(busy || model.sessionLink.isReady)
					Button(role: .destructive) {
						run { await model.disconnectBluetooth() }
					} label: {
						Label("断开蓝牙", systemImage: "xmark.circle")
					}
				} else {
					Button {
						run { await model.reconnect(camera) }
					} label: {
						Label("连接蓝牙", systemImage: "dot.radiowaves.left.and.right")
					}
					.disabled(busy || model.bluetoothAvailability != .ready)
				}
			}
			Menu {
				ForEach(CameraTransport.allCases, id: \.self) { t in
					Button(t.label) { model.beginManualWiFi(transport: t) }
				}
			} label: {
				Label("手动 Wi-Fi 连接…", systemImage: "hand.tap")
			}
			.disabled(busy)
			if model.sessionLink.isReady || model.sessionLink.isWorking {
				Button(role: .destructive) {
					run { await model.endSession(reason: "已手动断开") }
				} label: {
					Label("断开相机会话", systemImage: "stop.circle")
				}
			} else if case .failed = model.sessionLink, model.wifiLink.isReady {
				Button {
					run { await model.startSession(transport: model.activeCamera?.preferredTransport ?? .xappWirelessComm) }
				} label: {
					Label("重新建立会话", systemImage: "arrow.clockwise")
				}
			}
		}
	}

	@ViewBuilder private var cameraInfoSection: some View {
		Section("相机信息") {
			KeyValueRow(key: "名称（PTP）", value: model.sessionState?.cameraName.nilIfEmptyString)
			KeyValueRow(key: "型号（PTP DeviceInfo）", value: model.ptpDeviceInfo?.model)
			KeyValueRow(key: "固件（PTP DeviceInfo）", value: model.ptpDeviceInfo?.deviceVersion)
			KeyValueRow(key: "型号（蓝牙 2A24）", value: model.bleDevice?.model)
			KeyValueRow(key: "固件（蓝牙 2A26）", value: model.bleDevice?.firmware)
			KeyValueRow(key: "镜头（蓝牙）", value: model.bleDevice?.lensName)
			KeyValueRow(key: "电量", value: model.batteryText)
			KeyValueRow(key: "存储", value: model.storageText)
			if let s = model.sessionState {
				KeyValueRow(key: "相机状态", value: s.cameraState?.label ?? "\(s.cameraStateRaw)")
				KeyValueRow(key: "连接方式", value: s.transport?.label)
				KeyValueRow(key: "文件数", value: s.numObjects >= 0 ? "\(s.numObjects)" : nil)
				DisclosureGroup("协议版本") {
					KeyValueRow(key: "DF24 RemoteVersion", value: hex(s.remoteVersion))
					KeyValueRow(key: "DF21 ImageGetVersion", value: hex(s.imageGetVersion))
					KeyValueRow(key: "DF22 GetObjectVersion", value: hex(s.getObjectVersion))
					KeyValueRow(key: "DF25 RemoteGetObjectVersion", value: hex(s.remoteImageViewVersion))
					KeyValueRow(key: "DF28 RemotePhotoViewEx", value: hex(s.remoteImageViewExVersion))
				}
			}
			Button("刷新电量与存储") { run { await model.refreshCameraStatus() } }
		}
	}

	private func hex(_ v: Int32) -> String? { v < 0 ? nil : "0x" + String(v, radix: 16, uppercase: true) }

	private func run(_ work: @escaping @MainActor () async -> Void) {
		busy = true
		Task {
			await work()
			busy = false
		}
	}
}

extension String {
	var nilIfEmptyString: String? { isEmpty ? nil : self }
}

struct PairedCameraDetail: View {
	@Environment(AppModel.self) private var model
	@Environment(\.dismiss) private var dismiss
	let cameraID: UUID
	@State private var confirmForget = false

	var body: some View {
		if let camera = model.pairings.camera(cameraID) {
			List {
				Section("配对") {
					KeyValueRow(key: "名称", value: camera.displayName)
					KeyValueRow(key: "配对方式", value: camera.scheme.label)
					KeyValueRow(key: "蓝牙标识（本机专用 UUID）", value: camera.id.uuidString)
					KeyValueRow(key: "型号（蓝牙/PTP）", value: [camera.bleModel, camera.ptpModel].compactMap { $0 }.joined(separator: " / ").nilIfEmptyString)
					KeyValueRow(key: "固件（蓝牙/PTP）", value: [camera.bleFirmware, camera.ptpFirmware].compactMap { $0 }.joined(separator: " / ").nilIfEmptyString)
					KeyValueRow(key: "相机 Wi-Fi", value: camera.wifiSSID)
					KeyValueRow(key: "首次配对", value: camera.createdAt.formatted(date: .abbreviated, time: .shortened))
				}
				Section {
					Picker("Wi-Fi 连接方式", selection: Binding(
						get: { camera.preferredTransport },
						set: { t in model.pairings.update(cameraID) { $0.preferredTransport = t } }
					)) {
						ForEach(CameraTransport.allCases, id: \.self) { Text($0.label).tag($0) }
					}
				} footer: {
					Text("蓝牙开启的 Wi-Fi 使用 XApp 方式（libfuji 在相机 RemoteVersion 低于 0x20007 时自动回退为旧方式）。")
				}
				Section("连接记录") {
					if camera.history.isEmpty {
						Text("暂无记录").foregroundStyle(.secondary)
					}
					ForEach(camera.history) { r in
						VStack(alignment: .leading, spacing: 2) {
							HStack {
								Image(systemName: r.success ? "checkmark.circle" : "xmark.octagon")
									.foregroundStyle(r.success ? .green : .orange)
								Text(label(r.link))
								Spacer()
								Text(r.date.formatted(date: .abbreviated, time: .shortened)).font(.caption).foregroundStyle(.secondary)
							}
							Text(r.detail).font(.footnote).foregroundStyle(.secondary)
						}
					}
				}
				Section {
					Button("忘记此相机", role: .destructive) { confirmForget = true }
				} footer: {
					Text("会删除本 App 保存的配对令牌、序列号与 Wi-Fi 密码。若相机使用安全配对，还需在 iPhone 设置 > 蓝牙 中对该相机点按“忽略此设备”，并在相机上删除本机的配对。")
				}
			}
			.navigationTitle(camera.displayName)
			.confirmationDialog("忘记 \(camera.displayName)？", isPresented: $confirmForget, titleVisibility: .visible) {
				Button("忘记", role: .destructive) {
					Task {
						await model.forget(camera)
						dismiss()
					}
				}
			} message: {
				Text("此操作会删除配对资料与 Wi-Fi 密码，之后需要重新配对。已导入的照片不受影响。")
			}
		} else {
			ContentUnavailableView("相机已删除", systemImage: "camera")
		}
	}

	private func label(_ link: ConnectionRecord.Link) -> String {
		switch link {
		case .bluetooth: "蓝牙"
		case .wifi: "Wi-Fi"
		case .session: "会话"
		}
	}
}
