import SwiftUI
import UIKit
import FujiKit

/// Scan for Fujifilm advertisements and pair. The scheme (legacy token vs
/// secure bond) is decided from the camera's GATT services during pairing.
struct PairingSheet: View {
	@Environment(AppModel.self) private var model
	@Environment(\.dismiss) private var dismiss
	@State private var pairing: UUID?

	var body: some View {
		NavigationStack {
			List {
				Section {
					LinkStatusRow(title: "蓝牙", systemImage: "dot.radiowaves.left.and.right", status: model.bluetoothLink)
				}
				Section {
					Text("1. 在相机上打开蓝牙，并进入配对登记界面（例如“网络/USB 设置”或“连接设置”中的蓝牙/智能手机设置，菜单名以相机为准）。")
					Text("2. 在下方列表中点按显示为“可配对”的相机。")
					Text("3. 若出现系统蓝牙配对请求，请核对相机上的提示后点按“配对”；相机端如要求确认，也请确认。")
				} header: {
					Text("步骤")
				}
				.font(.footnote)

				Section {
					if model.discovered.isEmpty {
						HStack {
							ProgressView()
							Text(model.isScanning ? "正在扫描富士相机…" : "未开始扫描").foregroundStyle(.secondary)
						}
					}
					ForEach(model.discovered.sorted { $0.rssi > $1.rssi }) { camera in
						Button {
							pairing = camera.id
							Task {
								await model.pair(with: camera)
								pairing = nil
								if model.bluetoothLink.isReady { dismiss() }
							}
						} label: {
							HStack {
								VStack(alignment: .leading, spacing: 2) {
									Text(camera.name).font(.headline)
									Text(kindLabel(camera.advertisement)).font(.footnote).foregroundStyle(.secondary)
								}
								Spacer()
								if pairing == camera.id {
									ProgressView()
								} else {
									Text("\(camera.rssi) dBm").font(.caption).foregroundStyle(.secondary)
								}
							}
						}
						.disabled(pairing != nil || !(camera.advertisement.isPairable || model.pairings.camera(camera.id) != nil))
					}
				} header: {
					Text("附近的富士相机")
				} footer: {
					Text("仅显示厂商数据为富士（0x04D8）的设备。iOS 不提供蓝牙 MAC 地址，App 使用系统分配的设备标识保存配对。")
				}
			}
			.navigationTitle("配对相机")
			.toolbar {
				ToolbarItem(placement: .cancellationAction) {
					Button("关闭") { dismiss() }
				}
				ToolbarItem(placement: .primaryAction) {
					if model.isScanning {
						Button("停止") { model.stopScan() }
					} else {
						Button("扫描") { model.startScan() }
							.disabled(model.bluetoothAvailability != .ready)
					}
				}
			}
			.onAppear { if model.bluetoothAvailability == .ready { model.startScan() } }
			.onChange(of: model.bluetoothAvailability) { _, a in if a == .ready && !model.isScanning { model.startScan() } }
			.onDisappear { model.stopScan() }
		}
	}

	private func kindLabel(_ ad: FujiBLE.Advertisement) -> String {
		switch ad.kind {
		case .legacyPairing: return "可配对 · 旧版令牌方式"
		case .secure(_, let pairingMode): return pairingMode ? "可配对 · 安全配对方式" : "已配对过的相机正在广播（可重连）"
		case .unrecognised(let length, let type):
			return "富士设备，但广播格式未知（\(length) 字节，类型 \(type.map { String(format: "0x%02X", $0) } ?? "?")），暂不支持配对"
		}
	}
}

/// Manual join: the user joins the camera's Wi-Fi in Settings, then continues.
struct ManualJoinSheet: View {
	@Environment(AppModel.self) private var model
	@Environment(\.dismiss) private var dismiss
	@Environment(\.openURL) private var openURL
	let request: ManualJoinRequest
	@State private var transport: CameraTransport
	@State private var showPassword = false

	init(request: ManualJoinRequest) {
		self.request = request
		_transport = State(initialValue: request.transport)
	}

	var body: some View {
		NavigationStack {
			Form {
				Section {
					Text(request.reason).foregroundStyle(.secondary)
				}
				Section("相机 Wi-Fi") {
					if let ssid = request.ssid {
						LabeledContent("网络名称") {
							Text(ssid).textSelection(.enabled)
						}
						Button("拷贝网络名称") { UIPasteboard.general.string = ssid }
					} else {
						Text("请先在相机上开启无线通信，相机会显示网络名称（通常以 FUJIFILM- 开头）。")
					}
					if let password = request.password {
						LabeledContent("密码") {
							Text(showPassword ? password : String(repeating: "•", count: max(8, password.count)))
								.textSelection(.enabled)
						}
						Toggle("显示密码", isOn: $showPassword)
						Button("拷贝密码") { UIPasteboard.general.string = password }
					}
				}
				Section {
					Text("打开 设置 > 无线局域网，加入上面的网络。相机热点没有互联网，若 iOS 询问，请选择继续使用该网络（移动数据可保持开启）。加入后回到本 App 点按“我已加入”。")
						.font(.footnote)
					Button("打开“设置”") {
						if let url = URL(string: UIApplication.openSettingsURLString) { openURL(url) }
					}
				} footer: {
					Text("iOS 不允许 App 直接打开无线局域网设置页面，此按钮打开本 App 的设置。")
				}
				Section {
					Picker("连接方式", selection: $transport) {
						ForEach(CameraTransport.allCases, id: \.self) { Text($0.label).tag($0) }
					}
				} footer: {
					Text("由蓝牙开启的 Wi-Fi 选择“XApp 方式”；在相机菜单中开启的无线通信选择“相机菜单发起”。")
				}
				Section {
					Button {
						let t = transport
						dismiss()
						Task { await model.continueAfterManualJoin(transport: t) }
					} label: {
						Label("我已加入，继续", systemImage: "arrow.right.circle.fill")
					}
				}
			}
			.navigationTitle("手动加入 Wi-Fi")
			.toolbar {
				ToolbarItem(placement: .cancellationAction) {
					Button("取消") { dismiss() }
				}
			}
		}
	}
}
