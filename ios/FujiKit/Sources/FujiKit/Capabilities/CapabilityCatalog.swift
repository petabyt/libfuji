import Foundation

/// Single source for the capability matrix shown in the app (设置 > 能力矩阵)
/// and rendered to ios/docs/CAPABILITY_MATRIX.md by the test
/// `CapabilityCatalogTests.renderMarkdown` (FUJIKIT_WRITE_MATRIX=path).
public struct CapabilityEntry: Sendable, Identifiable, Hashable {
	public enum Status: String, Sendable, CaseIterable {
		case sourceImplemented = "源码已有实现"
		case portable = "理论可移植"
		case emulatorVerified = "vcam 仿真验证通过（非真机）"
		case deviceVerified = "X-T30 III 真机验证通过"
		case pending = "待验证"
		case unsupported = "不支持"
	}

	public enum AppState: String, Sendable {
		case implemented = "已实现"
		case implementedNeedsDevice = "已实现，待真机验证"
		case closed = "入口关闭"
		case notApplicable = "—"
	}

	public var id: String { feature }
	public let group: String
	public let feature: String
	public let source: String
	public let connection: String
	public let camera: String
	public let ios: String
	public let evidence: String
	public let statuses: [Status]
	public let app: AppState
}

public enum CapabilityCatalog {
	public static let libfujiSHA = "90aaa9319ef84711ef584fb4440d6e0d2511f662"
	public static let libpictSHA = "2e973f0336a2a58d749496ae9bd3795f12228241"
	public static let furbleSHA = "0cac22aacc3d9ef250be40849118af78a5f39f27"

	public static let entries: [CapabilityEntry] = [
		// MARK: Bluetooth
		.init(group: "蓝牙", feature: "扫描识别相机",
			  source: "libfuji.json bluetoothFilters（厂商数据 d8 04）；furble Fujifilm::matches",
			  connection: "BLE", camera: "待验证（零售商规格：蓝牙 5.2）",
			  ios: "可行：CoreBluetooth 前台扫描，按公司 ID 0x04D8 过滤",
			  evidence: "广播解析单元测试（BLEProtocolTests）", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "旧版令牌配对",
			  source: "lib/bluetooth.c fuji_connect_bluetooth（91F1DE68/ABA356EB）；furble FujifilmBasic",
			  connection: "BLE", camera: "未知：furble 称 2025-07 之后的固件改用安全配对",
			  ios: "可行：普通 GATT 写入，无需系统配对",
			  evidence: "源码对照；运行时按 GATT 服务判定，不与安全配对混用", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "安全配对（系统绑定）",
			  source: "lib/bluetooth.c 安全分支（123D8F06/F557D96B 状态回写 0x20）；furble FujifilmSecure",
			  connection: "BLE", camera: "可能（2025-11 发售机型），待验证",
			  ios: "可行：读取加密特征触发系统配对；iOS 无 createBond API",
			  evidence: "源码对照；ATT/CB 错误映射", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "客户端身份登记与通知订阅",
			  source: "send_client_name；subscribe 列表；furble subscription0/1",
			  connection: "BLE", camera: "待验证",
			  ios: "可行：setNotifyValue 自动写 CCCD（通知/指示）",
			  evidence: "订阅表与上游一致（必需/可选按 furble）", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "保存连接、重连、切换、忘记",
			  source: "furble 保存设备流程；libfuji PakSavedConnection",
			  connection: "BLE", camera: "待验证",
			  ios: "可行：用 CBPeripheral.identifier（非 MAC）+ 钥匙串保存令牌/序列号；安全配对需用户在系统设置中忽略设备",
			  evidence: "PairingStore / KeychainSecretStore", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "配对失效 / 配对目标不匹配提示",
			  source: "bluetooth.c TODO（SELECT PAIRING DESTINATION）",
			  connection: "BLE", camera: "待验证",
			  ios: "可行：CBError.peerRemovedPairingInformation、令牌写入失败",
			  evidence: "BLEError 映射", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "通过蓝牙开启相机 Wi-Fi",
			  source: "bluetooth.c fuji_bluetooth_connect_to_wifi（SSID/密码特征，600655E6 写 04 00，IND1 状态）",
			  connection: "BLE → Wi-Fi", camera: "待验证（上游注释：XApp/旧版判定尚不确定）",
			  ios: "可行", evidence: "仅 libfuji 有此流程，furble 无", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "遥控快门（对焦/按下/释放）",
			  source: "bluetooth.c send_shutter_command；furble sendShutterCommand",
			  connection: "BLE", camera: "待验证", ios: "可行", evidence: "报文单元测试",
			  statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "前台间隔拍摄",
			  source: "module.c 声明 PAK_SCREEN_INTERVALOMETER；定时由 App 实现",
			  connection: "BLE", camera: "待验证", ios: "仅前台（不保活后台）", evidence: "—",
			  statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "时间同步",
			  source: "bluetooth.c device_callback：收到 NOT1 写入 B9BFD37F（fujifilm_time_t）",
			  connection: "BLE", camera: "待验证", ios: "可行", evidence: "报文单元测试",
			  statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "蓝牙", feature: "位置同步",
			  source: "furble sendGeoData（纬经度×1e7）；libfuji update_geo_data 写固定测试坐标（未采用）",
			  connection: "BLE", camera: "待验证",
			  ios: "可行：仅前台，开启时申请“使用期间”定位权限，从不发送伪造坐标",
			  evidence: "报文单元测试", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),

		// MARK: Wi-Fi / PTP
		.init(group: "Wi-Fi/PTP", feature: "加入相机 Wi-Fi",
			  source: "fudge Android WifiNetworkSpecifier（仅参考）",
			  connection: "Wi-Fi", camera: "待验证",
			  ios: "条件可行：NEHotspotConfiguration 需 Hotspot Configuration 权限；否则手动加入。apply 成功≠已连接，App 以路由+TCP 55740 验证",
			  evidence: "HotspotJoiner / CameraNetwork", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "PTP/IP 会话建立",
			  source: "fuji.c ptpip_fuji_connect_handshake / fuji_setup；libpict ip.c（BSD socket）",
			  connection: "Wi-Fi 192.168.0.1:55740", camera: "待验证",
			  ios: "可行：BSD socket + SO_NOSIGPIPE + IP_BOUND_IF；需本地网络权限",
			  evidence: "vcam X-S10 / GFX100 II 仿真：建立会话、等待确认中止", statuses: [.sourceImplemented, .emulatorVerified, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "图库枚举与元数据",
			  source: "ptp_fuji_get_object_handles（句柄=1…N）；ptp_get_object_info",
			  connection: "Wi-Fi", camera: "待验证",
			  ios: "可行", evidence: "vcam 仿真；句柄非永久标识，按身份重新解析",
			  statuses: [.sourceImplemented, .emulatorVerified, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "缩略图",
			  source: "fuji.c fuji_get_thumb", connection: "Wi-Fi", camera: "待验证",
			  ios: "可行", evidence: "vcam 仿真（含 EXIF 缩略图的 JPEG）",
			  statuses: [.sourceImplemented, .emulatorVerified, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "JPEG 完整下载",
			  source: "fuji_download_file_ex（App 以 fujiios_download 逐步复刻其属性序列）",
			  connection: "Wi-Fi", camera: "待验证（XApp 模式 D226=2 是否即原图需真机确认）",
			  ios: "可行", evidence: "vcam：3.9 MB 多块下载逐字节一致；取消返回 CANCELED（已修复上游覆盖错误码）",
			  statuses: [.sourceImplemented, .emulatorVerified, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "HEIF 下载",
			  source: "同上（格式无关）", connection: "Wi-Fi", camera: "待验证：无线模式是否列出 HEIF 未知",
			  ios: "可行（存入照片取决于系统 HEIF 支持）", evidence: "—", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "RAF 下载（无线）",
			  source: "docs/dev.md：“与普通 Wi-Fi 配对不同，AutoSave 可下载 RAF”",
			  connection: "Wi-Fi", camera: "大概率不支持（智能手机无线模式通常不列出 RAF）",
			  ios: "如相机列出即可下载；RAW 内嵌预览≠RAF", evidence: "上游文档推断，待验证",
			  statuses: [.pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "视频下载",
			  source: "docs/help.md：旧机型传 JPEG 与 MOV", connection: "Wi-Fi",
			  camera: "待验证", ios: "可行；PTP GetPartialObject 32 位偏移限制 4 GB",
			  evidence: "—", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "缩图开关（相机缩小后传输）",
			  source: "fuji_download_file_ex：XApp 模式设置 D226=1/2，结束后设 2",
			  connection: "Wi-Fi（XApp 方式）", camera: "待验证",
			  ios: "可行；记录下载前后 D226/D227 值供核查；缩小文件单独标记",
			  evidence: "vcam 不实现 D226/D227（读回为不可读）", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "断点续传",
			  source: "PTP GetPartialObject 偏移读取（上游从 0 下载，App 增加起始偏移）",
			  connection: "Wi-Fi", camera: "待验证",
			  ios: "可行：续传前读回末尾 64 KB 比对，一致才续传，否则重下",
			  evidence: "vcam：偏移读取一致；FakeCamera 单元测试覆盖不一致/不支持", statuses: [.portable, .emulatorVerified, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "实时取景",
			  source: "fuji_config_liveview / ptp_fuji_read_liveview_frame（55741/55742）",
			  connection: "Wi-Fi", camera: "待验证",
			  ios: "可行", evidence: "vcam 读到 JPEG 帧；vcam 第二次 OpenCapture 会自行退出（仿真器限制）；上游读帧死循环已修补",
			  statuses: [.sourceImplemented, .emulatorVerified, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "Wi-Fi 远程拍摄",
			  source: "libfuji 无拍摄流程（fuji.c 未调用 InitiateCapture 等）",
			  connection: "Wi-Fi", camera: "未知", ios: "—",
			  evidence: "源码审查", statuses: [.unsupported], app: .closed),
		.init(group: "Wi-Fi/PTP", feature: "取景/图库模式切换",
			  source: "module.c on_switch_screen：fuji_end_liveview + fuji_config_image_gallery",
			  connection: "Wi-Fi", camera: "待验证", ios: "可行；传输进行中禁止切换",
			  evidence: "fuji_setup 内首次切换在 vcam 通过", statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "曝光/ISO/白平衡/胶片模拟读取",
			  source: "PTP GetDevicePropDesc；fujiptp.h 属性码（部分来自 libgphoto2，上游注明可能不准）",
			  connection: "Wi-Fi", camera: "待验证",
			  ios: "可行：Swift 安全解析（libpict 的 ptp_parse_prop_desc 遇 NoForm 会 abort）",
			  evidence: "描述符解析单元测试", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "修改拍摄参数",
			  source: "PTP SetDevicePropValue", connection: "Wi-Fi", camera: "待验证",
			  ios: "可行：仅当描述符 GetSet=1 且给出枚举/范围时开放，写后读回确认",
			  evidence: "—", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "电量 / 存储状态",
			  source: "PTP 0x5001、fujiptp.h D36A；GetStorageInfo", connection: "Wi-Fi",
			  camera: "待验证", ios: "可行；电量只报档位，不换算百分比", evidence: "—",
			  statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "相机端选图推送（标记传输）",
			  source: "fuji_get_events：camera_state=MULTIPLE_TRANSFER 时 app_queue_file_for_download(1)（上游 TODO）",
			  connection: "Wi-Fi", camera: "待验证", ios: "可行", evidence: "源码部分实现",
			  statuses: [.sourceImplemented, .pending], app: .implementedNeedsDevice),
		.init(group: "Wi-Fi/PTP", feature: "拍摄后自动传输",
			  source: "需蓝牙后台唤醒 + 自动切换 Wi-Fi，libfuji 无完整流程",
			  connection: "BLE + Wi-Fi", camera: "未知", ios: "受后台限制，不承诺",
			  evidence: "—", statuses: [.unsupported], app: .closed),
		.init(group: "Wi-Fi/PTP", feature: "删除相机照片",
			  source: "仅 tether 流程调用 ptp_delete_object", connection: "Wi-Fi", camera: "未知",
			  ios: "—", evidence: "首版不实现", statuses: [.unsupported], app: .closed),

		// MARK: other modes
		.init(group: "其他模式", feature: "PC AutoSave",
			  source: "lib/discovery.c（UDP 51541/51542 + TCP 51540）",
			  connection: "本地 Wi-Fi（相机连路由器）",
			  camera: "大概率不支持：上游文档称 2022 年后机型移除",
			  ios: "需向 Apple 申请组播 entitlement 才能收发广播", evidence: "源码与文档",
			  statuses: [.sourceImplemented, .unsupported], app: .closed),
		.init(group: "其他模式", feature: "无线联机拍摄（X Acquire）",
			  source: "discovery.c tether + fuji_usb.c fujitether_setup",
			  connection: "本地 Wi-Fi", camera: "未知：论坛称 X-T30 系列无联机拍摄（非官方）",
			  ios: "需组播 entitlement", evidence: "—", statuses: [.sourceImplemented, .pending], app: .closed),
		.init(group: "其他模式", feature: "USB 导入",
			  source: "libfuji fuji_usb.c 基于 libusb（iOS 不可用）",
			  connection: "USB（相机 USB 读卡器模式）", camera: "待验证",
			  ios: "可行：ImageCaptureCore（iOS 13+ 公共 API），与 Wi-Fi 共用去重/校验流程",
			  evidence: "Apple 文档：ICDeviceBrowser / ICCameraFile.requestReadData", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "其他模式", feature: "USB PTP 诊断（只读）",
			  source: "fuji_usb.c fujiusb_setup 读取 D16E 判断模式",
			  connection: "USB", camera: "待验证",
			  ios: "可行：ICCameraDevice.requestSendPTPCommand（GetDeviceInfo、读 D16E）",
			  evidence: "Apple 文档", statuses: [.portable, .pending], app: .implementedNeedsDevice),
		.init(group: "其他模式", feature: "USB 联机拍摄",
			  source: "fujiusb_setup（USB 模式 5）", connection: "USB", camera: "未知（X-T30 系列可能无此模式）",
			  ios: "理论上可经 PTP 透传实现；不能照搬 libusb", evidence: "—", statuses: [.sourceImplemented, .portable], app: .closed),
		.init(group: "其他模式", feature: "相机内 RAW 转换",
			  source: "fuji_usb.c fuji_process_raf（USB RAW CONV，0x900C/0x900D，D185）",
			  connection: "USB", camera: "官方 X RAW STUDIO 列为兼容，待验证",
			  ios: "理论可移植（PTP 透传需支持数据写出），首版未实现", evidence: "—",
			  statuses: [.sourceImplemented, .portable], app: .closed),
		.init(group: "其他模式", feature: "设置备份 / 恢复",
			  source: "fujiusb_download_backup / fujiusb_restore_backup（USB RAW CONV 模式）",
			  connection: "USB", camera: "待验证", ios: "理论可移植；恢复属写入操作，首版不实现",
			  evidence: "—", statuses: [.sourceImplemented, .portable], app: .closed),
		.init(group: "其他模式", feature: "配方文件解析（FP1/FP2/FP3）",
			  source: "fp/src xml.c fp_parse_fp1（本地文件，不涉及相机）",
			  connection: "无", camera: "不涉及", ios: "可行",
			  evidence: "fp 样例文件单元测试；已修补畸形 XML 崩溃", statuses: [.sourceImplemented, .emulatorVerified], app: .implemented),
		.init(group: "其他模式", feature: "固件更新 / 未知命令写入 / 上游测试套件",
			  source: "fujiptp.h 0x9040 等；lib/tester.c", connection: "—", camera: "—",
			  ios: "—", evidence: "首版明确不实现、不编译 tester.c", statuses: [.unsupported], app: .closed),
	]

	public static func markdown() -> String {
		var out = """
		# 能力矩阵（X-T30 III / iOS 27）

		本文件由 `CapabilityCatalog.swift` 生成（`FUJIKIT_WRITE_MATRIX=… swift test --filter renderMarkdown`），与 App 内“设置 > 能力矩阵”一致。

		- 上游：libfuji `\(libfujiSHA)`，libpict `\(libpictSHA)`，furble `\(furbleSHA)`（仅对照）
		- “vcam 仿真验证通过”指在 petabyt/vcam 模拟的 X-S10 / GFX100 II 上通过，**不等于** X-T30 III 真机验证。
		- 截至本提交，**没有任何一项完成 X-T30 III 真机验证**。

		| 分组 | 功能 | 源码/函数 | 连接模式 | X-T30 III | iOS 可行性 | 证据 | 状态 | App |
		|---|---|---|---|---|---|---|---|---|

		"""
		for e in entries {
			let cells = [e.group, e.feature, e.source, e.connection, e.camera, e.ios, e.evidence,
						 e.statuses.map(\.rawValue).joined(separator: "；"), e.app.rawValue]
			out += "| " + cells.map { $0.replacingOccurrences(of: "|", with: "\\|") }.joined(separator: " | ") + " |\n"
		}
		return out
	}
}
