import SwiftUI
import UIKit
import FujiKit

struct PhotoDetailView: View {
	@Environment(AppModel.self) private var model
	let handle: UInt32
	@State private var exporting: [URL]?

	var body: some View {
		let item = model.gallery.item(handle)
		let info = item?.info
		let local = localURL(item)
		List {
			Section {
				preview(item: item, local: local)
					.frame(maxWidth: .infinity)
					.listRowInsets(EdgeInsets())
			} footer: {
				if local == nil {
					Text("显示的是相机提供的缩略图，不是原图。")
				}
			}

			Section("文件") {
				KeyValueRow(key: "文件名", value: info?.filename)
				KeyValueRow(key: "格式", value: info.map { "\($0.format.displayName)（PTP 0x\(String($0.objectFormat, radix: 16, uppercase: true))）" })
				KeyValueRow(key: "大小（相机报告）", value: info.map { UInt64($0.compressedSize).bytesText })
				KeyValueRow(key: "尺寸", value: info.flatMap { $0.imageWidth > 0 ? "\($0.imageWidth) × \($0.imageHeight)" : nil })
				KeyValueRow(key: "拍摄时间", value: info?.dateCreated.map { $0.formatted(date: .long, time: .standard) + "（相机时间）" })
				KeyValueRow(key: "相机附加信息", value: info?.keywords.nilIfEmptyString)
				KeyValueRow(key: "当前列表位置（句柄）", value: "\(handle)")
				KeyValueRow(key: "导入状态", value: importText(item?.importStatus))
				if case .alreadyImported(let r)? = item?.importStatus {
					KeyValueRow(key: "已接收字节", value: "\(r.byteCount)")
					VStack(alignment: .leading, spacing: 2) {
						Text("SHA-256").font(.caption).foregroundStyle(.secondary)
						Text(r.sha256).font(.caption.monospaced()).textSelection(.enabled)
					}
				}
				if let e = item?.infoError { Text(e).font(.footnote).foregroundStyle(.orange) }
			}

			Section {
				ForEach(exifItems(item: item, local: local)) { e in
					KeyValueRow(key: e.label, value: e.value == "未知" ? nil : e.value)
				}
			} header: {
				Text("EXIF")
			} footer: {
				Text(local != nil ? "读取自已导入的原始文件。" : "下载前只能读取缩略图中携带的少量信息；下载后显示原文件的完整 EXIF。")
			}

			Section {
				Button {
					Task { await model.download(handles: [handle]) }
				} label: {
					Label("下载原图", systemImage: "arrow.down.circle")
				}
				.disabled(!model.capabilities.download.isAvailable || info == nil)
				if let local {
					ShareLink(item: local) { Label("分享", systemImage: "square.and.arrow.up") }
					Button { exporting = [local] } label: { Label("导出到“文件”…", systemImage: "folder") }
					Text("已保存在“文件”App：我的 iPhone > CamBridge > \(local.deletingLastPathComponent().pathComponents.suffix(3).joined(separator: "/"))")
						.font(.footnote).foregroundStyle(.secondary)
				}
			}
		}
		.navigationTitle(info?.filename ?? "照片")
		.navigationBarTitleDisplayMode(.inline)
		.task { if item?.info == nil { await model.gallery.loadInfo(handle, priority: .high) } }
		.sheet(item: Binding(get: { exporting.map(ExportItem.init) }, set: { exporting = $0?.urls })) { e in
			DocumentExporter(urls: e.urls) { exporting = nil }
		}
	}

	private struct ExportItem: Identifiable {
		let urls: [URL]
		var id: String { urls.map(\.path).joined() }
	}

	private func localURL(_ item: GalleryModel.Item?) -> URL? {
		guard let status = item?.importStatus, case .alreadyImported(let r) = status, let rel = r.localRelativePath else { return nil }
		let url = model.paths.documents.appendingPathComponent(rel)
		return FileManager.default.fileExists(atPath: url.path) ? url : nil
	}

	@ViewBuilder private func preview(item: GalleryModel.Item?, local: URL?) -> some View {
		if let local, let image = ImageDecoding.thumbnail(fileAt: local, maxPixel: 2048) {
			Image(uiImage: image).resizable().scaledToFit()
		} else if let data = item?.thumbnail, let image = ImageDecoding.thumbnail(data, maxPixel: 1024) {
			Image(uiImage: image).resizable().scaledToFit()
		} else {
			ContentUnavailableView("没有预览", systemImage: "photo")
		}
	}

	private func exifItems(item: GalleryModel.Item?, local: URL?) -> [ExifReader.Item] {
		if let local { return ExifReader.items(fileAt: local) }
		if let thumb = item?.thumbnail { return ExifReader.items(data: thumb) }
		return []
	}

	private func importText(_ s: DuplicateStatus?) -> String? {
		switch s {
		case .alreadyImported(let r)?:
			return "已导入 \(r.importedAt.formatted(date: .abbreviated, time: .shortened))" + (r.isCameraReduced ? "（相机缩小版）" : "") + (r.photosLocalIdentifier != nil ? "，已存入照片" : "")
		case .possibleDuplicate?:
			return "可能已导入（同名同大小，但相机未提供拍摄时间）"
		case .new?:
			return "未导入"
		case nil:
			return nil
		}
	}
}
