import SwiftUI
import FujiKit

/// Camera properties. A property appears only if GetDevicePropDesc returns a
/// valid descriptor; editing is offered only when the descriptor is writable
/// and lists the allowed values. Every write is confirmed and read back.
struct PropertiesView: View {
	@Environment(AppModel.self) private var model
	@State private var rows: [Row] = []
	@State private var loading = false
	@State private var pendingWrite: PendingWrite?
	@State private var message: String?

	struct Row: Identifiable {
		let property: KnownProperty
		var descriptor: PropertyDescriptor?
		var error: String?
		var id: UInt16 { property.code }
	}

	struct PendingWrite: Identifiable {
		let property: KnownProperty
		let descriptor: PropertyDescriptor
		let value: PTPValue
		var id: String { "\(property.code)-\(value)" }
	}

	var body: some View {
		List {
			if let message {
				Section { Text(message).font(.footnote) }
			}
			Section {
				ForEach(rows.filter { $0.descriptor != nil }) { row in
					propertyRow(row)
				}
			} header: {
				Text("相机报告的属性")
			} footer: {
				Text("值的含义仅在 PTP 标准定义时翻译；富士扩展属性显示原始值。属性码来自 libfuji fujiptp.h，其中部分源自 libgphoto2，上游注明可能不准确。")
			}
			let missing = rows.filter { $0.descriptor == nil }
			if !missing.isEmpty {
				Section("相机未提供") {
					ForEach(missing) { row in
						LabeledContent(row.property.name, value: row.error ?? "不支持")
							.font(.footnote)
					}
				}
			}
		}
		.navigationTitle("拍摄参数")
		.overlay { if loading { ProgressView("正在读取…") } }
		.task { await load() }
		.refreshable { await load() }
		.confirmationDialog("修改相机设置", isPresented: Binding(get: { pendingWrite != nil }, set: { if !$0 { pendingWrite = nil } }),
							titleVisibility: .visible, presenting: pendingWrite) { w in
			Button("写入 \(w.property.format(w.value, descriptor: w.descriptor))") { Task { await write(w) } }
		} message: { w in
			Text("将把相机的“\(w.property.name)”（0x\(String(w.property.code, radix: 16, uppercase: true))）从 \(w.property.format(w.descriptor.current, descriptor: w.descriptor)) 改为 \(w.property.format(w.value, descriptor: w.descriptor))。写入后会重新读取确认。")
		}
	}

	@ViewBuilder private func propertyRow(_ row: Row) -> some View {
		if let d = row.descriptor {
			if d.isEditableWithKnownValues {
				Menu {
					ForEach(d.allowedValues, id: \.self) { v in
						Button(row.property.format(v, descriptor: d)) {
							pendingWrite = PendingWrite(property: row.property, descriptor: d, value: v)
						}
						.disabled(v == d.current)
					}
				} label: {
					LabeledContent(row.property.name, value: row.property.format(d.current, descriptor: d))
				}
			} else {
				LabeledContent {
					Text(row.property.format(d.current, descriptor: d))
				} label: {
					VStack(alignment: .leading) {
						Text(row.property.name)
						Text(d.writable ? "可写，但相机未列出允许值" : "只读").font(.caption).foregroundStyle(.secondary)
					}
				}
			}
		}
	}

	private func load() async {
		guard let client = model.client else { return }
		loading = true
		defer { loading = false }
		var result: [Row] = []
		for p in KnownProperty.all {
			do {
				result.append(Row(property: p, descriptor: try await client.propertyDescriptor(p.code)))
			} catch let e as PTPError where e.isConnectionLost {
				message = e.localizedDescription
				break
			} catch {
				result.append(Row(property: p, descriptor: nil, error: error.localizedDescription))
			}
		}
		rows = result
	}

	private func write(_ w: PendingWrite) async {
		pendingWrite = nil
		guard let client = model.client else { return }
		do {
			let bytes = try PropertyDescriptor.encode(w.value, as: w.descriptor.dataType)
			try await client.setPropertyRaw(w.property.code, bytes: bytes)
			let after = try await client.propertyDescriptor(w.property.code)
			if let after, after.current == w.value {
				message = "“\(w.property.name)”已改为 \(w.property.format(after.current, descriptor: after))（已读回确认）"
			} else {
				message = "相机接受了写入，但读回的值为 \(after.map { w.property.format($0.current, descriptor: $0) } ?? "未知")"
			}
			await load()
		} catch {
			message = "写入失败：\(error.localizedDescription)"
		}
	}
}
