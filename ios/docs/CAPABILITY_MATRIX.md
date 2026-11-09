# 能力矩阵（X-T30 III / iOS 27）

本文件由 `CapabilityCatalog.swift` 生成（`FUJIKIT_WRITE_MATRIX=… swift test --filter renderMarkdown`），与 App 内“设置 > 能力矩阵”一致。

- 上游：libfuji `90aaa9319ef84711ef584fb4440d6e0d2511f662`，libpict `2e973f0336a2a58d749496ae9bd3795f12228241`，furble `0cac22aacc3d9ef250be40849118af78a5f39f27`（仅对照）
- “vcam 仿真验证通过”指在 petabyt/vcam 模拟的 X-S10 / GFX100 II 上通过，**不等于** X-T30 III 真机验证。
- 截至本提交，**没有任何一项完成 X-T30 III 真机验证**。

| 分组 | 功能 | 源码/函数 | 连接模式 | X-T30 III | iOS 可行性 | 证据 | 状态 | App |
|---|---|---|---|---|---|---|---|---|
| 蓝牙 | 扫描识别相机 | libfuji.json bluetoothFilters（厂商数据 d8 04）；furble Fujifilm::matches | BLE | 待验证（零售商规格：蓝牙 5.2） | 可行：CoreBluetooth 前台扫描，按公司 ID 0x04D8 过滤 | 广播解析单元测试（BLEProtocolTests） | 源码已有实现；待验证 | 已实现，待真机验证 |
| 蓝牙 | 旧版令牌配对 | lib/bluetooth.c fuji_connect_bluetooth（91F1DE68/ABA356EB）；furble FujifilmBasic | BLE | 未知：furble 称 2025-07 之后的固件改用安全配对 | 可行：普通 GATT 写入，无需系统配对 | 源码对照；运行时按 GATT 服务判定，不与安全配对混用 | 源码已有实现；待验证 | 已实现，待真机验证 |
| 蓝牙 | 安全配对（系统绑定） | lib/bluetooth.c 安全分支（123D8F06/F557D96B 状态回写 0x20）；furble FujifilmSecure | BLE | 可能（2025-11 发售机型），待验证 | 可行：读取加密特征触发系统配对；iOS 无 createBond API | 源码对照；ATT/CB 错误映射 | 源码已有实现；待验证 | 已实现，待真机验证 |
| 蓝牙 | 客户端身份登记与通知订阅 | send_client_name；subscribe 列表；furble subscription0/1 | BLE | 待验证 | 可行：setNotifyValue 自动写 CCCD（通知/指示） | 订阅表与上游一致（必需/可选按 furble） | 源码已有实现；待验证 | 已实现，待真机验证 |
| 蓝牙 | 保存连接、重连、切换、忘记 | furble 保存设备流程；libfuji PakSavedConnection | BLE | 待验证 | 可行：用 CBPeripheral.identifier（非 MAC）+ 钥匙串保存令牌/序列号；安全配对需用户在系统设置中忽略设备 | PairingStore / KeychainSecretStore | 理论可移植；待验证 | 已实现，待真机验证 |
| 蓝牙 | 配对失效 / 配对目标不匹配提示 | bluetooth.c TODO（SELECT PAIRING DESTINATION） | BLE | 待验证 | 可行：CBError.peerRemovedPairingInformation、令牌写入失败 | BLEError 映射 | 理论可移植；待验证 | 已实现，待真机验证 |
| 蓝牙 | 通过蓝牙开启相机 Wi-Fi | bluetooth.c fuji_bluetooth_connect_to_wifi（SSID/密码特征，600655E6 写 04 00，IND1 状态） | BLE → Wi-Fi | 待验证（上游注释：XApp/旧版判定尚不确定） | 可行 | 仅 libfuji 有此流程，furble 无 | 源码已有实现；待验证 | 已实现，待真机验证 |
| 蓝牙 | 遥控快门（对焦/按下/释放） | bluetooth.c send_shutter_command；furble sendShutterCommand | BLE | 待验证 | 可行 | 报文单元测试 | 源码已有实现；待验证 | 已实现，待真机验证 |
| 蓝牙 | 前台间隔拍摄 | module.c 声明 PAK_SCREEN_INTERVALOMETER；定时由 App 实现 | BLE | 待验证 | 仅前台（不保活后台） | — | 理论可移植；待验证 | 已实现，待真机验证 |
| 蓝牙 | 时间同步 | bluetooth.c device_callback：收到 NOT1 写入 B9BFD37F（fujifilm_time_t） | BLE | 待验证 | 可行 | 报文单元测试 | 源码已有实现；待验证 | 已实现，待真机验证 |
| 蓝牙 | 位置同步 | furble sendGeoData（纬经度×1e7）；libfuji update_geo_data 写固定测试坐标（未采用） | BLE | 待验证 | 可行：仅前台，开启时申请“使用期间”定位权限，从不发送伪造坐标 | 报文单元测试 | 源码已有实现；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 加入相机 Wi-Fi | fudge Android WifiNetworkSpecifier（仅参考） | Wi-Fi | 待验证 | 条件可行：NEHotspotConfiguration 需 Hotspot Configuration 权限；否则手动加入。apply 成功≠已连接，App 以路由+TCP 55740 验证 | HotspotJoiner / CameraNetwork | 理论可移植；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | PTP/IP 会话建立 | fuji.c ptpip_fuji_connect_handshake / fuji_setup；libpict ip.c（BSD socket） | Wi-Fi 192.168.0.1:55740 | 待验证 | 可行：BSD socket + SO_NOSIGPIPE + IP_BOUND_IF；需本地网络权限 | vcam X-S10 / GFX100 II 仿真：建立会话、等待确认中止 | 源码已有实现；vcam 仿真验证通过（非真机）；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 图库枚举与元数据 | ptp_fuji_get_object_handles（句柄=1…N）；ptp_get_object_info | Wi-Fi | 待验证 | 可行 | vcam 仿真；句柄非永久标识，按身份重新解析 | 源码已有实现；vcam 仿真验证通过（非真机）；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 缩略图 | fuji.c fuji_get_thumb | Wi-Fi | 待验证 | 可行 | vcam 仿真（含 EXIF 缩略图的 JPEG） | 源码已有实现；vcam 仿真验证通过（非真机）；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | JPEG 完整下载 | fuji_download_file_ex（App 以 fujiios_download 逐步复刻其属性序列） | Wi-Fi | 待验证（XApp 模式 D226=2 是否即原图需真机确认） | 可行 | vcam：3.9 MB 多块下载逐字节一致；取消返回 CANCELED（已修复上游覆盖错误码） | 源码已有实现；vcam 仿真验证通过（非真机）；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | HEIF 下载 | 同上（格式无关） | Wi-Fi | 待验证：无线模式是否列出 HEIF 未知 | 可行（存入照片取决于系统 HEIF 支持） | — | 理论可移植；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | RAF 下载（无线） | docs/dev.md：“与普通 Wi-Fi 配对不同，AutoSave 可下载 RAF” | Wi-Fi | 大概率不支持（智能手机无线模式通常不列出 RAF） | 如相机列出即可下载；RAW 内嵌预览≠RAF | 上游文档推断，待验证 | 待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 视频下载 | docs/help.md：旧机型传 JPEG 与 MOV | Wi-Fi | 待验证 | 可行；PTP GetPartialObject 32 位偏移限制 4 GB | — | 理论可移植；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 缩图开关（相机缩小后传输） | fuji_download_file_ex：XApp 模式设置 D226=1/2，结束后设 2 | Wi-Fi（XApp 方式） | 待验证 | 可行；记录下载前后 D226/D227 值供核查；缩小文件单独标记 | vcam 不实现 D226/D227（读回为不可读） | 源码已有实现；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 断点续传 | PTP GetPartialObject 偏移读取（上游从 0 下载，App 增加起始偏移） | Wi-Fi | 待验证 | 可行：续传前读回末尾 64 KB 比对，一致才续传，否则重下 | vcam：偏移读取一致；FakeCamera 单元测试覆盖不一致/不支持 | 理论可移植；vcam 仿真验证通过（非真机）；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 实时取景 | fuji_config_liveview / ptp_fuji_read_liveview_frame（55741/55742） | Wi-Fi | 待验证 | 可行 | vcam 读到 JPEG 帧；vcam 第二次 OpenCapture 会自行退出（仿真器限制）；上游读帧死循环已修补 | 源码已有实现；vcam 仿真验证通过（非真机）；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | Wi-Fi 远程拍摄 | libfuji 无拍摄流程（fuji.c 未调用 InitiateCapture 等） | Wi-Fi | 未知 | — | 源码审查 | 不支持 | 入口关闭 |
| Wi-Fi/PTP | 取景/图库模式切换 | module.c on_switch_screen：fuji_end_liveview + fuji_config_image_gallery | Wi-Fi | 待验证 | 可行；传输进行中禁止切换 | fuji_setup 内首次切换在 vcam 通过 | 源码已有实现；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 曝光/ISO/白平衡/胶片模拟读取 | PTP GetDevicePropDesc；fujiptp.h 属性码（部分来自 libgphoto2，上游注明可能不准） | Wi-Fi | 待验证 | 可行：Swift 安全解析（libpict 的 ptp_parse_prop_desc 遇 NoForm 会 abort） | 描述符解析单元测试 | 理论可移植；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 修改拍摄参数 | PTP SetDevicePropValue | Wi-Fi | 待验证 | 可行：仅当描述符 GetSet=1 且给出枚举/范围时开放，写后读回确认 | — | 理论可移植；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 电量 / 存储状态 | PTP 0x5001、fujiptp.h D36A；GetStorageInfo | Wi-Fi | 待验证 | 可行；电量只报档位，不换算百分比 | — | 理论可移植；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 相机端选图推送（标记传输） | fuji_get_events：camera_state=MULTIPLE_TRANSFER 时 app_queue_file_for_download(1)（上游 TODO） | Wi-Fi | 待验证 | 可行 | 源码部分实现 | 源码已有实现；待验证 | 已实现，待真机验证 |
| Wi-Fi/PTP | 拍摄后自动传输 | 需蓝牙后台唤醒 + 自动切换 Wi-Fi，libfuji 无完整流程 | BLE + Wi-Fi | 未知 | 受后台限制，不承诺 | — | 不支持 | 入口关闭 |
| Wi-Fi/PTP | 删除相机照片 | 仅 tether 流程调用 ptp_delete_object | Wi-Fi | 未知 | — | 首版不实现 | 不支持 | 入口关闭 |
| 其他模式 | PC AutoSave | lib/discovery.c（UDP 51541/51542 + TCP 51540） | 本地 Wi-Fi（相机连路由器） | 大概率不支持：上游文档称 2022 年后机型移除 | 需向 Apple 申请组播 entitlement 才能收发广播 | 源码与文档 | 源码已有实现；不支持 | 入口关闭 |
| 其他模式 | 无线联机拍摄（X Acquire） | discovery.c tether + fuji_usb.c fujitether_setup | 本地 Wi-Fi | 未知：论坛称 X-T30 系列无联机拍摄（非官方） | 需组播 entitlement | — | 源码已有实现；待验证 | 入口关闭 |
| 其他模式 | USB 导入 | libfuji fuji_usb.c 基于 libusb（iOS 不可用） | USB（相机 USB 读卡器模式） | 待验证 | 可行：ImageCaptureCore（iOS 13+ 公共 API），与 Wi-Fi 共用去重/校验流程 | Apple 文档：ICDeviceBrowser / ICCameraFile.requestReadData | 理论可移植；待验证 | 已实现，待真机验证 |
| 其他模式 | USB PTP 诊断（只读） | fuji_usb.c fujiusb_setup 读取 D16E 判断模式 | USB | 待验证 | 可行：ICCameraDevice.requestSendPTPCommand（GetDeviceInfo、读 D16E） | Apple 文档 | 理论可移植；待验证 | 已实现，待真机验证 |
| 其他模式 | USB 联机拍摄 | fujiusb_setup（USB 模式 5） | USB | 未知（X-T30 系列可能无此模式） | 理论上可经 PTP 透传实现；不能照搬 libusb | — | 源码已有实现；理论可移植 | 入口关闭 |
| 其他模式 | 相机内 RAW 转换 | fuji_usb.c fuji_process_raf（USB RAW CONV，0x900C/0x900D，D185） | USB | 官方 X RAW STUDIO 列为兼容，待验证 | 理论可移植（PTP 透传需支持数据写出），首版未实现 | — | 源码已有实现；理论可移植 | 入口关闭 |
| 其他模式 | 设置备份 / 恢复 | fujiusb_download_backup / fujiusb_restore_backup（USB RAW CONV 模式） | USB | 待验证 | 理论可移植；恢复属写入操作，首版不实现 | — | 源码已有实现；理论可移植 | 入口关闭 |
| 其他模式 | 配方文件解析（FP1/FP2/FP3） | fp/src xml.c fp_parse_fp1（本地文件，不涉及相机） | 无 | 不涉及 | 可行 | fp 样例文件单元测试；已修补畸形 XML 崩溃 | 源码已有实现；vcam 仿真验证通过（非真机） | 已实现 |
| 其他模式 | 固件更新 / 未知命令写入 / 上游测试套件 | fujiptp.h 0x9040 等；lib/tester.c | — | — | — | 首版明确不实现、不编译 tester.c | 不支持 | 入口关闭 |
