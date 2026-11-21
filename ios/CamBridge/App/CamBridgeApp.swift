import SwiftUI
import FujiKit

@main
struct CamBridgeApp: App {
	@State private var model: AppModel
	@Environment(\.scenePhase) private var scenePhase

	init() {
		// Background continued-processing handlers are registered per batch when a
		// download starts (BackgroundTransferCoordinator), not at launch.
		_model = State(initialValue: AppModel())
	}

	var body: some Scene {
		WindowGroup {
			RootView()
				.environment(model)
				.onChange(of: scenePhase) { _, phase in
					Task { await model.scenePhaseChanged(phase) }
				}
		}
	}
}

struct RootView: View {
	@Environment(AppModel.self) private var model

	var body: some View {
		@Bindable var model = model
		TabView {
			Tab("相机", systemImage: "camera") {
				CameraView()
			}
			Tab("图库", systemImage: "photo.on.rectangle") {
				GalleryView()
			}
			Tab("传输", systemImage: "arrow.down.circle") {
				TransfersView()
			}
			.badge(model.queue.pendingCount)
			Tab("遥控", systemImage: "camera.shutter.button") {
				RemoteView()
			}
			Tab("设置", systemImage: "gearshape") {
				SettingsView()
			}
		}
		.alert(model.alert?.title ?? "", isPresented: Binding(
			get: { model.alert != nil },
			set: { if !$0 { model.alert = nil } }
		), presenting: model.alert) { _ in
			Button("好", role: .cancel) {}
		} message: { a in
			Text(a.message)
		}
		.sheet(item: $model.manualJoin) { request in
			ManualJoinSheet(request: request)
		}
	}
}
