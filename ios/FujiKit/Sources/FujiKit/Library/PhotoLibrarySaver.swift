#if canImport(Photos) && os(iOS)
import Foundation
import Photos

/// Adds downloaded originals to Photos from their file URL, so the camera's
/// bytes and metadata are stored unchanged (no UIImage decode/re-encode).
/// Only add-only access is requested; reading the library is never needed.
public struct PhotoLibrarySaver: MediaLibrarySaver {
	public init() {}

	public static var addOnlyStatus: PHAuthorizationStatus {
		PHPhotoLibrary.authorizationStatus(for: .addOnly)
	}

	public static func requestAddOnlyAccess() async -> PHAuthorizationStatus {
		await PHPhotoLibrary.requestAuthorization(for: .addOnly)
	}

	public func save(fileAt url: URL, format: MediaFormat, originalFilename: String, creationDate: Date?) async throws -> String {
		var status = PHPhotoLibrary.authorizationStatus(for: .addOnly)
		if status == .notDetermined {
			status = await PHPhotoLibrary.requestAuthorization(for: .addOnly)
		}
		guard status == .authorized || status == .limited else { throw PhotoSaveError.notAuthorized }

		let resourceType: PHAssetResourceType
		switch format {
		case .jpeg, .heif, .tiff:
			resourceType = .photo
		case .mov, .mp4:
			resourceType = .video
		case .raf:
			// Photos imports RAW files only for camera models the OS RAW engine
			// supports; if not, performChanges fails and the file stays in Files.
			resourceType = .photo
		case .unknown:
			throw PhotoSaveError.unsupportedFormat("未知格式")
		}

		let uti = format.uniformTypeIdentifier
		let placeholder = Locked<String?>(nil)
		do {
			try await PHPhotoLibrary.shared().performChanges {
				let options = PHAssetResourceCreationOptions()
				options.originalFilename = originalFilename
				options.shouldMoveFile = false // keep the copy in Documents for the Files app
				if let uti { options.uniformTypeIdentifier = uti }
				let request = PHAssetCreationRequest.forAsset()
				request.addResource(with: resourceType, fileURL: url, options: options)
				if let creationDate { request.creationDate = creationDate }
				placeholder.set(request.placeholderForCreatedAsset?.localIdentifier)
			}
		} catch {
			let ns = error as NSError
			if format == .raf {
				throw PhotoSaveError.unsupportedFormat("RAF：\(ns.localizedDescription)")
			}
			throw PhotoSaveError.failed(ns.localizedDescription)
		}
		guard let id = placeholder.value else { throw PhotoSaveError.failed("未返回资源标识") }
		return id
	}
}
#endif
