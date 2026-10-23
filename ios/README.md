# 相机桥（CamBridge）：FUJIFILM X-T30 III 的 iPhone App

基于 [petabyt/libfuji](https://github.com/petabyt/libfuji)（本仓库）与 libpict 的原生 iOS App，目标机型是 **FUJIFILM X-T30 III**（第三代，不是 X-T30 / X-T30 II）。

> **验证状态（务必先读）**
>
> - 本目录里的代码**没有在 X-T30 III 真机上运行过**。
> - 编写环境只有 Linux，做过的验证是：C 核心与平台无关的 Swift 代码在 Linux 上编译、跑单元测试，以及对开源仿真相机 petabyt/vcam（仿真 X-S10 / GFX100 II，不是 X-T30 III）做端到端测试。
> - 只依赖 Apple 框架的代码（CoreBluetooth、NetworkExtension、Photos、ImageCaptureCore、CoreLocation、BackgroundTasks、SwiftUI 界面）在编写环境中只做过 `swiftc -parse` 语法检查和人工审阅。
> - 使用者已在 Xcode（iOS 27 SDK）中成功构建并启动过本分支的早期版本，报告了 C 代码警告、一个弃用警告和后台任务注册失败，这些已在之后的提交中修正。修正后的版本尚未在 Xcode 中重新构建。
> - 每项功能的状态见 [docs/CAPABILITY_MATRIX.md](docs/CAPABILITY_MATRIX.md)，真机验证步骤见 [docs/X-T30-III-VERIFICATION.md](docs/X-T30-III-VERIFICATION.md)。

## 目录

```
ios/
├── CamBridge/                 SwiftUI App（中文界面，5 个标签页：相机/图库/传输/遥控/设置）
│   ├── App/                   AppModel（蓝牙/Wi-Fi/会话编排）、设置、图库模型、@main
│   ├── Views/                 各页面
│   └── Resources/             图标、第三方许可文本
├── CamBridge.xcodeproj        Xcode 工程（同步文件夹，本地 Swift 包依赖 FujiKit）
├── project.yml                XcodeGen 备用工程描述（与 .xcodeproj 等价）
├── Config/                    Info.plist、entitlements、Signing.xcconfig
├── FujiKit/                   Swift 包
│   ├── Sources/CFujiCore/     C 核心：固定版本的 libfuji/libpict/fp + 补丁 + iOS shim（模块映射）
│   ├── Sources/FujiKit/       Swift 层：PTP 会话、传输引擎/队列、蓝牙、Wi-Fi、照片、USB、记录、能力目录
│   └── Tests/                 Swift 单元测试、vcam 端到端测试、C 测试程序
├── patches/                   对上游 C 代码的补丁（vendor-sync.sh 按序应用）
├── scripts/                   vendor-sync.sh、c-core-check.sh、vcam-integration.sh
└── docs/                      能力矩阵、上游/补丁记录、协议笔记、真机验证清单
```

## 构建与运行（macOS + Xcode）

### 要求

- 装有 **iOS 27 SDK** 的**正式版 Xcode**（Xcode 27）。工程使用 Swift 6 语言模式，开启完整并发检查（`SWIFT_STRICT_CONCURRENCY = complete`）。
- 运行 **iOS 27** 的 iPhone。工程最低部署版本是 27.0，模拟器没有蓝牙，也连不上相机 Wi-Fi。
- Apple 开发者账号。用免费账号可以安装到自己的设备，但可能无法启用 Hotspot Configuration 能力，见下文。

### 步骤

1. 取得代码：

   ```sh
   git clone https://github.com/obsidianplusplus/Fujifilm.git
   cd Fujifilm
   git checkout claude/wonderful-keller-yl2261
   ```

   构建 App **不需要** libpict 子模块，C 源码已固定版本复制到 `ios/FujiKit/Sources/CFujiCore/upstream/`。只有运行 `vendor-sync.sh` 时才需要子模块，见下文。

2. 编辑 `ios/Config/Signing.xcconfig`：
   - `DEVELOPMENT_TEAM`：你的团队 ID。
   - `PRODUCT_BUNDLE_IDENTIFIER`：你账号下唯一的 ID，例如 `com.yourname.cambridge`。
   - `CODE_SIGN_ENTITLEMENTS`：
     - 账号能启用 **Hotspot Configuration** 时，保留 `Config/CamBridge.entitlements`。App 会用 `NEHotspotConfiguration` 自动加入相机 Wi-Fi。
     - 不能启用时，改为 `Config/CamBridge-NoHotspot.entitlements`。App 会弹出手动加入页，显示网络名称，并提供“拷贝网络名称”“拷贝密码”按钮，你在系统设置里加入后回到 App 继续。

3. 打开 `ios/CamBridge.xcodeproj`。也可以先运行 `cd ios && xcodegen generate`，用 `project.yml` 生成工程。选择 CamBridge scheme 和你的 iPhone，然后运行。

4. 首次运行时，系统会依次询问以下权限，App 只在用到时才请求：
   - **蓝牙**：扫描相机时。
   - **本地网络**：首次连接 192.168.0.1 时。
   - **照片（仅添加）**：首次存入照片时。
   - **定位（使用期间）**：仅在开启“向相机提供位置”后。

### 与 X-T30 III 首次配对（建议流程）

1. 相机：菜单 → 网络/USB 设置 → 蓝牙/智能手机设置 → **配对登记**。具体菜单名以相机固件为准。
2. App：相机 → **添加相机（蓝牙配对）** → 选中扫描到的机型 → 配对。
   - App 按相机实际暴露的 GATT 服务决定使用**旧版令牌配对**还是**安全配对（系统绑定）**，两条分支不混用。
   - 如果是安全配对，iOS 会弹出系统配对确认框。
3. 配对成功后，在“相机”页点 **通过蓝牙开启相机 Wi-Fi 并连接**。App 通过蓝牙请求相机开启 Wi-Fi、读取 SSID 和密码（密码只存钥匙串），然后加入热点，再做路由与 TCP 55740 探测，最后建立 PTP/IP 会话。
   - **“已应用 Wi-Fi 配置”不等于已连接**。界面上只有 PTP 会话建立后，“会话”一行才会显示已连接。
4. 照片只通过 Wi-Fi（PTP/IP）传输，不走蓝牙。

如果蓝牙流程失败，可以在相机上手动开启无线通信，然后在“相机”页选择 **手动 Wi-Fi 连接…**（相机菜单发起的无线通信，transport 2）。

## 在 Linux / CI 上可执行的检查

以下命令都在仓库根目录执行，不需要 Xcode。

| 检查 | 命令 | 覆盖范围 |
|---|---|---|
| 复制的 C 源码是否与固定上游 + 补丁一致 | `ios/scripts/vendor-sync.sh --check` | 逐文件比对 sha256（需要子模块，见下文） |
| C 核心 + iOS shim，ASan/UBSan | `ios/scripts/c-core-check.sh` | 用 clang 编译全部 C 代码（隐式声明，以及 Xcode 会报的 `-Wshorten-64-to-32`、`-Wunsequenced` 都视为错误），运行冒烟测试 |
| 对 vcam 仿真相机的 C 层集成测试 | `VCAM=/path/to/vcam ios/scripts/vcam-integration.sh` | 握手、OpenSession、事件、对象列表、分块下载、断点续传、中止 setup/下载、实时取景帧 |
| FujiKit Swift 单元测试 | `cd ios/FujiKit && docker run --rm -v "$PWD":/pkg -w /pkg swift:6.4-noble swift test` | 报文编解码、描述符解析、传输引擎（续传校验、去重、取消、断线等待）、队列、配方解析、能力矩阵 |
| Swift 端到端（PTPClient → TransferEngine → vcam） | 先启动 vcam（`fuji_x_s10 tcp --ip 127.0.0.1 --fs <card>`），再在 `ios/FujiKit` 中执行 `docker run --rm --network host -e FUJIKIT_VCAM_CARD=/card -v <card>:/card:ro -v "$PWD":/pkg -w /pkg swift:6.4-noble swift test --filter VCam` | 浏览、下载、完整性检查、导入记录 |
| 重新生成能力矩阵 | `docker run --rm -v "$PWD/ios":/ios -w /ios/FujiKit -e FUJIKIT_WRITE_MATRIX=/ios/docs/CAPABILITY_MATRIX.md swift:6.4-noble swift test --filter renderMarkdown` | 由 `CapabilityCatalog.swift` 生成 |

vcam 的构建方法见 `ios/scripts/vcam-integration.sh` 文件头注释。vcam 是 LGPL 许可的测试工具，**只在测试时使用，不随 App 分发**。

### 子模块的 SSH 地址

`.gitmodules` 中 libpict 的地址是 `git@github.com:petabyt/camlib.git`（SSH）。没有 GitHub SSH 密钥的环境无法拉取它。`vendor-sync.sh` 初始化子模块时，会用一次性的 `-c url.https://github.com/.insteadOf=git@github.com:` 改走 HTTPS，不修改 `.gitmodules`。手动拉取时：

```sh
git -c url."https://github.com/".insteadOf="git@github.com:" submodule update --init libpict
```

## 本次没有执行的构建与检查

| 未执行 | 原因 | 风险/后续 |
|---|---|---|
| 在编写环境中用 Xcode 27 / iOS 27 SDK 编译 | 编写环境只有 Linux，没有 macOS 和 Xcode | 使用者已能构建本分支的早期版本；之后对 `BackgroundTransferCoordinator` 的修改（改用 `submitTaskRequest`）只在 Linux 上用模拟类型做过 Swift 6 并发检查，请重新构建确认 |
| 在 iPhone 上运行 | 同上，也没有设备 | 权限弹窗、后台行为、界面布局都未实测 |
| X-T30 III 真机上的任何协议交互 | 没有相机 | 蓝牙配对方案、SSID 格式、传输模式、D226/D227 效果、RAF/HEIF/视频是否出现在 Wi-Fi 列表等全部待验证，见验证清单 |
| 上游 libfuji/libpict 自带的测试套件 | 按要求不自动执行可能含写入/删除的测试 | 只运行了本目录自己的测试，均为只读或针对仿真器 |
| App Store / TestFlight 构建、隐私清单审查 | 超出本版范围 | 上架前需补 `PrivacyInfo.xcprivacy` 并复核所需理由 API |
| 能否在 iOS 上用 ImageCaptureCore 访问 X-T30 III | 没有设备和 iOS SDK | 是否需要额外的 Info.plist 键待确认 |
| BGContinuedProcessingTask 是否需要额外的 UIBackgroundModes | Apple 文档未明确，未在设备上验证 | 当前未添加任何后台模式。如果系统拒绝提交，日志（分类 background）会记录原因，前台传输不受影响 |

## 设计要点

- **C 核心**：
  - 以 SwiftPM C target（`CFujiCore`）加模块映射引入。
  - 所有阻塞的 C 调用都在 `PTPWorker` 的专用线程上**按顺序**执行，PTP 事务不会交错；界面在 MainActor 上更新。
  - 取消时调用 `shutdown()` 打断阻塞的 socket 读取，描述符由工作线程关闭。
- **蓝牙**：
  - `BLECameraLink` 把所有 CoreBluetooth 调用限定在一个串行队列上，事件通过 `AsyncStream` 交给界面。
  - 已配对相机用 `CBPeripheral.identifier` 标识，不用 MAC 地址。
- **凭据**：配对令牌、相机序列号、Wi-Fi 密码只存在钥匙串（`kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly`）。日志写入前经过 `Redactor` 脱敏，导出的诊断日志也是脱敏后的版本。
- **传输**：
  - 分块写入临时文件；完成后校验长度和格式完整性（JPEG EOI、RAF 头、ISO BMFF box），计算 SHA-256 去重。
  - 存入“照片”时用 `PHAssetCreationRequest.addResource(with:fileURL:)` 直接写原始字节，不重新编码。原文件同时保存在 `Documents/导入/<相机>/<日期>/`，可在“文件”App 中查看。
  - **对象句柄不作为身份**：身份由文件名、大小、拍摄时间组成。续传前先重新按身份找到句柄，再读取续传点之前的 64 KB 与本地数据比对，一致才续传，否则从头下载。
  - “相机缩小后传输”（D226）得到的文件会单独标记，不与原图混淆。
- **后台**：
  - iOS 26 起，传输可以用 `BGContinuedProcessingTaskRequest`（系统显示进度，用户可取消），时间到了就暂停，保留已下载的数据，回到前台后自动继续（续传前会校验）。
  - Info.plist 只允许通配的 `<Bundle ID>.transfer.*`。每次开始下载时生成新的 `<Bundle ID>.transfer.<UUID>`，**先为这个具体标识注册处理器，再提交请求**。系统不允许为通配标识本身注册处理器（Apple DTS 在开发者论坛 thread 799126 中的答复），所以启动时不注册任何处理器。
  - **不使用后台音频等保活手段**；间隔拍摄只在前台运行。
- **危险操作**：
  - 首版不实现删除相机照片、恢复设置、固件刷写，也不尝试未知命令。
  - 属性写入只允许描述符里列出的值，写入前展示对象和影响并请用户确认，写入后读回核对。

## 许可

- **本仓库**：libfuji，MIT。
- **libpict**：Apache-2.0。
- **ezxml**：MIT（随 fp 复制）。
- **furble**：MIT，只作为蓝牙协议对照，未复制代码。
- **fudge**：未声明许可，**只阅读参考，没有复制任何代码**。

许可文本在 `CamBridge/Resources/Licenses/`，App 内可在“设置 → 开源许可”查看。上游版本和补丁详情见 [docs/UPSTREAM.md](docs/UPSTREAM.md)。
