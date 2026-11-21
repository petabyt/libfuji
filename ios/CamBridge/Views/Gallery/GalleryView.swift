import SwiftUI
import UIKit
import FujiKit

struct GalleryView: View {
	@Environment(AppModel.self) private var model
	@State private var selecting = false
	@State private var confirmRedownload: [UInt32] = []

	private let columns = [GridItem(.adaptive(minimum: 104), spacing: 2)]

	var body: some View {
		@Bindable var gallery = model.gallery
		NavigationStack {
			Group {
				if let reason = model.capabilities.gallery.reason, model.gallery.items.isEmpty {
					ContentUnavailableView {
						Label("图库不可用", systemImage: "photo.on.rectangle.angled")
					} description: {
						Text(model.gallery.message ?? reason)
					}
				} else if model.gallery.isLoading && model.gallery.items.isEmpty {
					ProgressView("正在读取相机文件列表…")
				} else {
					ScrollView {
						if model.gallery.totalCount > 0 && model.gallery.infoLoaded < model.gallery.totalCount {
							Text("已读取文件信息 \(model.gallery.infoLoaded)/\(model.gallery.totalCount)，排序会随之更新")
								.font(.footnote).foregroundStyle(.secondary).padding(.top, 4)
						}
						if model.gallery.isOffline && !model.gallery.items.isEmpty {
							Label(model.gallery.message ?? "未连接相机", systemImage: "wifi.slash")
								.font(.footnote).foregroundStyle(.orange).padding(.top, 4)
						}
						LazyVGrid(columns: columns, spacing: 2) {
							ForEach(model.gallery.displayed) { item in
								cell(item)
							}
						}
					}
				}
			}
			.navigationTitle("图库")
			.navigationDestination(for: UInt32.self) { handle in
				PhotoDetailView(handle: handle)
			}
			.toolbar {
				ToolbarItem(placement: .topBarLeading) {
					Menu {
						Picker("排序", selection: $gallery.newestFirst) {
							Text("拍摄日期：最新在前").tag(true)
							Text("拍摄日期：最早在前").tag(false)
						}
						Picker("类型", selection: $gallery.filter) {
							ForEach(GalleryModel.Filter.allCases) { Text($0.rawValue).tag($0) }
						}
					} label: {
						Label("排序与筛选", systemImage: "line.3.horizontal.decrease.circle")
					}
				}
				ToolbarItem(placement: .topBarTrailing) {
					Button(selecting ? "完成" : "选择") {
						selecting.toggle()
						if !selecting { gallery.selection.removeAll() }
					}
					.disabled(model.gallery.items.isEmpty)
				}
				if selecting {
					ToolbarItemGroup(placement: .bottomBar) {
						Button("全选") { gallery.selection = Set(model.gallery.displayed.map(\.handle)) }
						Spacer()
						Text("已选 \(gallery.selection.count) 项").font(.footnote)
						Spacer()
						Button {
							let handles = Array(gallery.selection)
							let known = handles.filter {
								if case .alreadyImported = model.gallery.item($0)?.importStatus { return true }
								return false
							}
							if known.isEmpty {
								Task { await model.download(handles: handles) }
								selecting = false
							} else {
								confirmRedownload = handles
							}
						} label: {
							Label("下载", systemImage: "arrow.down.circle")
						}
						.disabled(gallery.selection.isEmpty || !model.capabilities.download.isAvailable)
					}
				}
			}
			.confirmationDialog("部分文件此前已导入", isPresented: Binding(get: { !confirmRedownload.isEmpty }, set: { if !$0 { confirmRedownload = [] } }),
								titleVisibility: .visible) {
				Button("跳过已导入的文件") {
					let handles = confirmRedownload
					confirmRedownload = []
					selecting = false
					Task { await model.download(handles: handles) }
				}
				Button("全部重新下载") {
					let handles = confirmRedownload
					confirmRedownload = []
					selecting = false
					Task { await model.download(handles: handles, force: true) }
				}
			} message: {
				Text("已导入的文件按相机、文件名、大小和拍摄时间识别。重新下载后若内容完全相同（SHA-256），仍不会重复保存。")
			}
			.refreshable {
				if let client = model.client, let state = model.sessionState {
					await model.gallery.reload(client: client, state: state, cameraID: model.cameraID, imports: model.imports)
				}
			}
		}
	}

	@ViewBuilder private func cell(_ item: GalleryModel.Item) -> some View {
		let selected = model.gallery.selection.contains(item.handle)
		let content = ZStack(alignment: .bottomLeading) {
			Thumbnail(item: item)
				.frame(minWidth: 0, maxWidth: .infinity)
				.aspectRatio(1, contentMode: .fill)
				.clipped()
			HStack(spacing: 4) {
				if let f = item.info?.format, f != .jpeg {
					Text(f == .raf ? "RAF" : f == .heif ? "HEIF" : f.isVideo ? "视频" : f.displayName)
						.font(.caption2.bold())
						.padding(.horizontal, 4).padding(.vertical, 1)
						.background(.ultraThinMaterial, in: .capsule)
				}
				if case .alreadyImported = item.importStatus {
					Image(systemName: "checkmark.circle.fill").foregroundStyle(.white, .green)
						.accessibilityLabel("已导入")
				}
			}
			.padding(4)
			if selecting {
				Image(systemName: selected ? "checkmark.circle.fill" : "circle")
					.font(.title3)
					.foregroundStyle(selected ? .white : .white.opacity(0.8), selected ? Color.accentColor : .clear)
					.frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topTrailing)
					.padding(6)
			}
		}
		.contentShape(.rect)
		.task(id: item.handle) { await model.gallery.loadThumbnail(item.handle) }

		if selecting {
			content.onTapGesture {
				if selected { model.gallery.selection.remove(item.handle) } else { model.gallery.selection.insert(item.handle) }
			}
		} else {
			NavigationLink(value: item.handle) { content }
				.buttonStyle(.plain)
		}
	}
}

struct Thumbnail: View {
	let item: GalleryModel.Item

	var body: some View {
		ZStack {
			Rectangle().fill(.quaternary)
			if let data = item.thumbnail, let image = ImageDecoding.thumbnail(data, maxPixel: 320) {
				Image(uiImage: image).resizable().scaledToFill()
			} else {
				switch item.thumbState {
				case .loading, .notLoaded:
					ProgressView()
				case .none:
					Image(systemName: item.info?.format.isVideo == true ? "video" : "photo").foregroundStyle(.secondary)
				case .failed:
					Image(systemName: "exclamationmark.triangle").foregroundStyle(.secondary)
				case .loaded:
					Image(systemName: "photo").foregroundStyle(.secondary)
				}
			}
		}
	}
}
