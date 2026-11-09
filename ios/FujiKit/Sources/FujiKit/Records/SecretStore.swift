import Foundation
#if canImport(Security)
import Security
#endif

/// Storage for pairing tokens, camera serials and Wi-Fi passwords.
public protocol SecretStore: Sendable {
	func set(_ data: Data?, for key: String) throws
	func get(_ key: String) -> Data?
}

public enum SecretStoreError: Error, LocalizedError {
	case keychain(Int32)
	public var errorDescription: String? {
		switch self {
		case .keychain(let status): "钥匙串写入失败（\(status)）"
		}
	}
}

#if canImport(Security)
/// Generic-password Keychain items, readable after first unlock and never
/// synced or migrated to another device (ThisDeviceOnly): a Wi-Fi password or
/// pairing token is tied to this phone's pairing with the camera.
public struct KeychainSecretStore: SecretStore {
	public let service: String

	public init(service: String = "CamBridge.camera-secrets") {
		self.service = service
	}

	public func set(_ data: Data?, for key: String) throws {
		let query: [String: Any] = [
			kSecClass as String: kSecClassGenericPassword,
			kSecAttrService as String: service,
			kSecAttrAccount as String: key,
		]
		SecItemDelete(query as CFDictionary)
		guard let data else { return }
		var add = query
		add[kSecValueData as String] = data
		add[kSecAttrAccessible as String] = kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly
		let status = SecItemAdd(add as CFDictionary, nil)
		guard status == errSecSuccess else { throw SecretStoreError.keychain(status) }
	}

	public func get(_ key: String) -> Data? {
		let query: [String: Any] = [
			kSecClass as String: kSecClassGenericPassword,
			kSecAttrService as String: service,
			kSecAttrAccount as String: key,
			kSecReturnData as String: true,
			kSecMatchLimit as String: kSecMatchLimitOne,
		]
		var item: CFTypeRef?
		guard SecItemCopyMatching(query as CFDictionary, &item) == errSecSuccess else { return nil }
		return item as? Data
	}
}
#endif

/// For tests and platforms without a Keychain.
public final class InMemorySecretStore: SecretStore {
	private let storage = Locked<[String: Data]>([:])
	public init() {}
	public func set(_ data: Data?, for key: String) throws {
		storage.withLock { $0[key] = data }
	}
	public func get(_ key: String) -> Data? {
		storage.withLock { $0[key] }
	}
}
