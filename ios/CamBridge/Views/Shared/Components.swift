import SwiftUI
import UIKit
import ImageIO
import FujiKit

/// One link (Bluetooth / Wi-Fi / session) with its own state.
struct LinkStatusRow: View {
	let title: String
	let systemImage: String
	let status: LinkStatus

	var body: some View {
		HStack(alignment: .top, spacing: 12) {
			Image(systemName: systemImage)
				.font(.title3)
				.foregroundStyle(color)
				.frame(width: 28)
			VStack(alignment: .leading, spacing: 2) {
				Text(title).font(.headline)
				Text(status.text)
					.font(.subheadline)
					.foregroundStyle(.secondary)
					.fixedSize(horizontal: false, vertical: true)
			}
			Spacer()
			if status.isWorking {
				ProgressView()
			} else {
				Image(systemName: icon).foregroundStyle(color)
			}
		}
		.accessibilityElement(children: .combine)
	}

	private var color: Color {
		switch status {
		case .idle: .secondary
		case .working: .blue
		case .ready: .green
		case .failed: .orange
		}
	}

	private var icon: String {
		switch status {
		case .idle: "circle"
		case .working: "ellipsis.circle"
		case .ready: "checkmark.circle.fill"
		case .failed: "exclamationmark.triangle.fill"
		}
	}
}

/// A feature that is closed in this session, with the actual reason.
struct UnavailableNote: View {
	let title: String
	let reason: String

	var body: some View {
		Label {
			VStack(alignment: .leading, spacing: 2) {
				Text(title)
				Text(reason).font(.footnote).foregroundStyle(.secondary)
			}
		} icon: {
			Image(systemName: "nosign").foregroundStyle(.secondary)
		}
	}
}

struct KeyValueRow: View {
	let key: String
	let value: String?

	var body: some View {
		LabeledContent(key) {
			Text(value ?? "未知")
				.foregroundStyle(value == nil ? .tertiary : .secondary)
				.multilineTextAlignment(.trailing)
				.textSelection(.enabled)
		}
	}
}

enum ImageDecoding {
	/// Downsampled decode for display only (saved files are never re-encoded).
	static func thumbnail(_ data: Data, maxPixel: CGFloat) -> UIImage? {
		guard let src = CGImageSourceCreateWithData(data as CFData, nil) else { return nil }
		return thumbnail(src, maxPixel: maxPixel)
	}

	static func thumbnail(fileAt url: URL, maxPixel: CGFloat) -> UIImage? {
		guard let src = CGImageSourceCreateWithURL(url as CFURL, nil) else { return nil }
		return thumbnail(src, maxPixel: maxPixel)
	}

	private static func thumbnail(_ src: CGImageSource, maxPixel: CGFloat) -> UIImage? {
		let options: [CFString: Any] = [
			kCGImageSourceCreateThumbnailFromImageAlways: true,
			kCGImageSourceCreateThumbnailWithTransform: true,
			kCGImageSourceThumbnailMaxPixelSize: maxPixel,
		]
		guard let cg = CGImageSourceCreateThumbnailAtIndex(src, 0, options as CFDictionary) else { return nil }
		return UIImage(cgImage: cg)
	}
}

/// "导出到文件": copies files with the system document picker (UIDocumentPickerViewController asCopy).
struct DocumentExporter: UIViewControllerRepresentable {
	let urls: [URL]
	var onFinish: () -> Void = {}

	func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
		let picker = UIDocumentPickerViewController(forExporting: urls, asCopy: true)
		picker.delegate = context.coordinator
		return picker
	}

	func updateUIViewController(_ controller: UIDocumentPickerViewController, context: Context) {}

	func makeCoordinator() -> Coordinator { Coordinator(onFinish: onFinish) }

	final class Coordinator: NSObject, UIDocumentPickerDelegate {
		let onFinish: () -> Void
		init(onFinish: @escaping () -> Void) { self.onFinish = onFinish }
		func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) { onFinish() }
		func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) { onFinish() }
	}
}

extension Double {
	var speedText: String {
		guard self > 0 else { return "—" }
		return ByteCountFormatter.string(fromByteCount: Int64(self), countStyle: .file) + "/秒"
	}
}

extension UInt64 {
	var bytesText: String { ByteCountFormatter.string(fromByteCount: Int64(clamping: self), countStyle: .file) }
}
