import SwiftUI
import UniformTypeIdentifiers
import FujiKit

/// Parses X RAW STUDIO recipe files locally with libfuji's fp library.
/// This does not talk to the camera and does not apply the recipe.
struct RecipeViewerView: View {
	@State private var importing = false
	@State private var recipe: Recipe?
	@State private var fileName: String?
	@State private var error: String?

	var body: some View {
		List {
			Section {
				Button("选择配方文件…") { importing = true }
			} footer: {
				Text("仅解析文件内容（FP1/FP2/FP3 为 X RAW STUDIO 的 XML 配方）。不会读取或修改相机设置，也不会执行相机内 RAW 转换。")
			}
			if let error {
				Section { Text(error).foregroundStyle(.orange) }
			}
			if let recipe {
				Section(fileName ?? "配方") {
					if !recipe.complete {
						Text("部分取值超出 fp 已知的取值表，未能全部显示。").font(.footnote).foregroundStyle(.orange)
					}
					ForEach(recipe.entries) { e in
						LabeledContent(e.label, value: e.value)
					}
				}
			}
		}
		.navigationTitle("配方文件")
		.fileImporter(isPresented: $importing, allowedContentTypes: [.xml, .data, .item]) { result in
			switch result {
			case .success(let url): parse(url)
			case .failure(let e): error = e.localizedDescription
			}
		}
	}

	private func parse(_ url: URL) {
		error = nil
		recipe = nil
		let scoped = url.startAccessingSecurityScopedResource()
		defer { if scoped { url.stopAccessingSecurityScopedResource() } }
		let ext = url.pathExtension.uppercased()
		guard ["FP1", "FP2", "FP3", "XML"].contains(ext) else {
			error = "不是配方文件（扩展名应为 FP1、FP2 或 FP3）"
			return
		}
		// Copy into the sandbox first: fp opens the path with plain fopen().
		let local = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString + "." + ext)
		do {
			try FileManager.default.copyItem(at: url, to: local)
			defer { try? FileManager.default.removeItem(at: local) }
			let attrs = try FileManager.default.attributesOfItem(atPath: local.path)
			if let size = attrs[.size] as? NSNumber, size.intValue > 1_000_000 {
				error = "文件过大，不像配方文件"
				return
			}
			recipe = try Recipe.parse(fileAt: local)
			fileName = url.lastPathComponent
		} catch {
			self.error = error.localizedDescription
		}
	}
}
