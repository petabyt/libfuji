import Foundation
#if canImport(CryptoKit)
import CryptoKit
#endif

/// Incremental SHA-256 used to fingerprint downloaded files for duplicate
/// detection. Uses CryptoKit on Apple platforms and a portable implementation
/// elsewhere (Linux test runs).
public struct SHA256Hasher: Sendable {
	#if canImport(CryptoKit)
	private var impl = CryptoKit.SHA256()
	#else
	private var impl = PortableSHA256()
	#endif

	public init() {}

	public mutating func update(_ data: UnsafeRawBufferPointer) {
		impl.update(bufferPointer: data)
	}

	public mutating func update(_ data: Data) {
		data.withUnsafeBytes { update($0) }
	}

	public func finalizeHex() -> String {
		#if canImport(CryptoKit)
		return impl.finalize().map { String(format: "%02x", $0) }.joined()
		#else
		return impl.finalize().hexString
		#endif
	}

	public static func hex(of data: Data) -> String {
		var h = SHA256Hasher()
		h.update(data)
		return h.finalizeHex()
	}

	/// Hashes a file in 1 MiB blocks without loading it into memory.
	public static func hex(ofFileAt url: URL) throws -> String {
		let handle = try FileHandle(forReadingFrom: url)
		defer { try? handle.close() }
		var h = SHA256Hasher()
		while true {
			let chunk = try handle.read(upToCount: 1 << 20) ?? Data()
			if chunk.isEmpty { break }
			h.update(chunk)
		}
		return h.finalizeHex()
	}
}

/// FIPS 180-4 SHA-256. Verified against the NIST test vectors in the unit tests.
struct PortableSHA256: Sendable {
	private static let k: [UInt32] = [
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
	]

	private var h: [UInt32] = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19]
	private var buffer: [UInt8] = []
	private var length: UInt64 = 0

	mutating func update(bufferPointer: UnsafeRawBufferPointer) {
		length &+= UInt64(bufferPointer.count)
		var input = bufferPointer[...]
		if !buffer.isEmpty {
			let need = 64 - buffer.count
			let take = min(need, input.count)
			buffer.append(contentsOf: input.prefix(take))
			input = input.dropFirst(take)
			if buffer.count == 64 {
				let block = buffer
				buffer.removeAll(keepingCapacity: true)
				block.withUnsafeBytes { compress($0[...]) }
			}
		}
		while input.count >= 64 {
			compress(input.prefix(64))
			input = input.dropFirst(64)
		}
		buffer.append(contentsOf: input)
	}

	func finalize() -> [UInt8] {
		var copy = self
		let bitLength = length &* 8
		var tail = copy.buffer
		tail.append(0x80)
		while tail.count % 64 != 56 { tail.append(0) }
		for i in (0..<8).reversed() { tail.append(UInt8((bitLength >> (UInt64(i) * 8)) & 0xff)) }
		copy.buffer.removeAll()
		tail.withUnsafeBytes { raw in
			var off = 0
			while off < raw.count {
				copy.compress(raw[off..<(off + 64)])
				off += 64
			}
		}
		var out: [UInt8] = []
		for word in copy.h {
			out.append(UInt8(word >> 24)); out.append(UInt8((word >> 16) & 0xff))
			out.append(UInt8((word >> 8) & 0xff)); out.append(UInt8(word & 0xff))
		}
		return out
	}

	private mutating func compress(_ block: Slice<UnsafeRawBufferPointer>) {
		var w = [UInt32](repeating: 0, count: 64)
		var i = block.startIndex
		for t in 0..<16 {
			w[t] = UInt32(block[i]) << 24 | UInt32(block[i + 1]) << 16 | UInt32(block[i + 2]) << 8 | UInt32(block[i + 3])
			i += 4
		}
		for t in 16..<64 {
			let s0 = rotr(w[t - 15], 7) ^ rotr(w[t - 15], 18) ^ (w[t - 15] >> 3)
			let s1 = rotr(w[t - 2], 17) ^ rotr(w[t - 2], 19) ^ (w[t - 2] >> 10)
			w[t] = w[t - 16] &+ s0 &+ w[t - 7] &+ s1
		}
		var a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7]
		for t in 0..<64 {
			let s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)
			let ch = (e & f) ^ (~e & g)
			let t1 = hh &+ s1 &+ ch &+ PortableSHA256.k[t] &+ w[t]
			let s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)
			let maj = (a & b) ^ (a & c) ^ (b & c)
			let t2 = s0 &+ maj
			hh = g; g = f; f = e; e = d &+ t1; d = c; c = b; b = a; a = t1 &+ t2
		}
		h[0] &+= a; h[1] &+= b; h[2] &+= c; h[3] &+= d; h[4] &+= e; h[5] &+= f; h[6] &+= g; h[7] &+= hh
	}

	private func rotr(_ x: UInt32, _ n: UInt32) -> UInt32 { (x >> n) | (x << (32 - n)) }
}
