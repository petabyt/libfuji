# 上游来源、固定版本与补丁

iOS App 的 C 核心通过 `ios/scripts/vendor-sync.sh` 从**固定的 git 对象**复制而来，不读取工作区文件，然后按顺序应用 `ios/patches/` 中的补丁。

- 结果存放在 `ios/FujiKit/Sources/CFujiCore/upstream/`。
- `VENDOR_MANIFEST.txt` 记录了每个文件的 sha256。
- `vendor-sync.sh --check` 会重新生成一份并逐字节比对。

## 固定版本

| 组件 | 地址 | 提交 | 日期 | 许可 | 用途 |
|---|---|---|---|---|---|
| libfuji | 本仓库 obsidianplusplus/Fujifilm（历史来自 petabyt/libfuji） | `90aaa9319ef84711ef584fb4440d6e0d2511f662` | 2026-10-10 | MIT | 复制 `lib/` 中的 PTP 部分和 `fp/src/`，打补丁后编译进 App |
| libpict（子模块 `libpict`，原名 camlib） | `git@github.com:petabyt/camlib.git` | `2e973f0336a2a58d749496ae9bd3795f12228241` | 2026-09-07 | Apache-2.0 | 复制 `src/` 中的 PTP/IP 部分，打补丁后编译进 App |
| ezxml | 随 libfuji `fp/src/` | 同 libfuji | — | MIT | 配方 XML 解析 |
| furble | https://github.com/gkoh/furble | `0cac22aacc3d9ef250be40849118af78a5f39f27` | — | MIT | **只做对照**：蓝牙特征 UUID、订阅列表、安全配对流程。没有复制代码 |
| fudge | https://github.com/petabyt/fudge | `e1ff829e784d3d4d72e1cf51de83c668edebcf5b` | — | 仓库**未声明许可** | **只阅读参考**：Android 端的连接、下载流程。没有复制任何代码 |
| vcam | https://github.com/petabyt/vcam | `010e33a8639ff276688cd6f75b70b899bedfbc58` | — | LGPL | **仅测试**：仿真富士相机（X-S10 / GFX100 II），不随 App 分发，也不链接进 App |

关于子模块：
- 本仓库在 `90aaa93` 记录的 libpict gitlink 就是 `2e973f0`，`git submodule status` 无偏移。
- 子模块地址是 SSH 形式。`vendor-sync.sh` 用一次性的 `-c url."https://github.com/".insteadOf="git@github.com:"` 走 HTTPS 拉取，不修改 `.gitmodules`。

## 复制的文件

完整清单见 `vendor-sync.sh` 中的 `FILES` 数组。

- **重命名**（SwiftPM 同一 target 内不能有同名的 `.c` 文件）：
  - `lib/data.c` 改为 `libfuji/fuji_data.c`。
  - libpict `src/data.c` 改为 `libpict/pict_data.c`。
  - `fp/src/data.c` 改为 `fp/fp_data.c`。
- **没有复制的文件**：
  - `lib/bluetooth.c`、`lib/module.c`：依赖本仓库之外的 pak 运行时（`runtime.h`、`bluetooth.h`、`module.h`，以及 `pak_bt_*` 接口）。蓝牙部分改用 Swift + CoreBluetooth 重新实现（`FujiKit/Bluetooth/`），报文格式逐项对照 `bluetooth.c` 与 furble，有单元测试。
  - libpict 的 libusb/Android 后端：iOS 没有 libusb，第三方 App 也不能占用任意 USB 端点。`shim/fujiios_usb_stub.c` 提供返回 `PTP_UNSUPPORTED` 的桩函数。USB 功能改用 ImageCaptureCore（`FujiKit/USB/`）。
- **iOS shim**（`shim/fujiios_shim.c`、`include/CFujiCore.h`）：
  - 实现 libfuji 要求的前端回调（`lib/app.h`）。
  - 设置 socket 选项：`SO_NOSIGPIPE`，以及 `IP_BOUND_IF`，用于把连接绑定到相机所在的 Wi-Fi 接口，避免走蜂窝网络。
  - 给 Swift 提供一组不透明句柄 API。
  - 用 `io_lock` 串行化“中止”和“关闭”。

## 补丁

全部补丁只修正明确的缺陷，不改变正常数据的行为。每个补丁的提交说明里有详细原因。

| 补丁 | 问题 | 后果（未修时） |
|---|---|---|
| libfuji/0001 `fuji_reset_ptp-free-previous-priv` | 条件写反（`if (r->priv == NULL) free(r->priv)`） | 每次重置会话都泄漏 `PtpUserPriv` |
| libfuji/0002 `fuji_get_events-bounds-unlock-copy` | 相机给出的事件数量不做边界检查；循环中新的 PTP 事务会覆盖 `r->data`；提前 `return` 时互斥锁未释放；`a->data[i]` 应为 `a->data[y]` | 越界读取、事件解析错乱、死锁 |
| libfuji/0003 `fuji_download_file_ex-keep-download-result` | 清理阶段把 `fuji_get_events()`/D227 复位的返回值写入 `rc` | 被取消或失败的下载被报告为成功，**半截文件可能被当成完整文件** |
| libfuji/0004 `ptp_get_partial_exif-unlock-on-error` | 两处提前返回未解锁 | 死锁。该函数（PC AutoSave 缩略图）在上游本身仍有问题，App 不调用 |
| libfuji/0005 `liveview-frame-read-fully-and-validate` | 头部长度 < 18 时 `len -= 18` 下溢；读到 0 字节时无限重试；头部没读满会让数据流错位 | 尝试读取约 4 GB、实时取景线程卡死 |
| libpict/0001 `ip-shutdown-io-and-comm-deinit` | 无法从其他线程打断阻塞的 `read()`；`ptpip_comm_deinit` 只有声明没有定义；connect 超时泄漏 fd | `fuji_setup` 等待用户在相机上按 OK 时最长可阻塞 255 × 5 秒，无法取消 |
| fp/0001 `fp_parse_fp1-null-checks-and-free` | 文件不可读或 XML 格式错误时解引用 NULL；解析树未释放；`device`/`version` 字段从未填充 | 打开损坏的配方文件就崩溃 |
| fp/0002 `ezxml-unsequenced-access-and-explicit-narrowing` | `ezxml.c:364` 的 `n = ++s + strspn(s, …)` 在没有序列点的情况下修改并读取 `s`（未定义行为）；另有两处有界的 64→32 位转换 | 结果依赖编译器的求值顺序。改用逗号运算符先自增，与原意一致 |
| libfuji/0006 `recvfrom-signed-result-and-explicit-narrowing` | `discovery.c`：`recvfrom()` 的结果存进 `unsigned int`，出错时的 -1 变成 0xFFFFFFFF，`len <= 0` 判断不到，随后 `greeting[len] = '\0'` 越界写入；`fuji_usb.c`：`ftell()` 失败（-1）时能通过“文件过大”检查 | 内存越界写入。这两处都不在 App 的调用路径上（PC AutoSave 发现、USB 恢复备份） |
| libpict/0002 `guard-32-bit-sizes-and-explicit-narrowing` | `ptp_buffer_resize` 的缓冲区按 64 位大小分配，长度却截断存入 32 位 `data_length`；MTP 字符串长度字节在超过 254 个字符时回绕；固件更新、CHDK 上传、`ptp_download_object` 没有检查 32 位上限 | 缓冲区长度记录错误。现在拒绝无法表示的大小，或显式截断 |

## 已知但未打补丁的上游问题

| 位置 | 问题 | App 中的处理 |
|---|---|---|
| libpict `ptp_parse_prop_desc` | 遇到 NoForm/DateTime/数组等表单类型会调用 `ptp_panic`；也不检查载荷长度 | **App 不调用它**：shim 只返回原始字节，由 Swift 的 `PropertyDescriptor` 做带边界检查的解析。解析失败的属性只显示原因；没有允许值列表（范围/枚举）的属性只读显示，都不提供写入 |
| libfuji `ptp_get_partial_exif` | 返回的偏移和长度指向一个临时缓冲区，而且该缓冲区泄漏 | 不调用；PC AutoSave 入口关闭 |
| libpict `ptpip_read_packet`（`transport.c`） | 网络数据包中的长度字段不经检查就参与无符号运算：长度小于已读字节数会下溢，接近 4 GB 时缓冲区计算会回绕 | 只用于标准 PTP/IP（`PTP_IP`）。富士连接使用 `PTP_IP_USB`，走 `ptpusb_read_all_packets`，该循环每次读取前都按“已读 + 最大包长”扩大缓冲区，不受影响。未修改 |
| libfuji `bluetooth.c` 中的 TODO | XApp/旧版协议判定、“SELECT PAIRING DESTINATION”不匹配、未绑定设备的处理 | Swift 实现按 GATT 服务判定，不混用两种方案；写入失败时提示检查相机的配对目标设置 |

## 与上游的行为差异

- `fujiios_download` 复刻了 `fuji_download_file_ex` 的流程（D227 设置与复位、分块 GetPartialObject），另外增加两项：
  - 起始偏移，用于续传；续传前会先做偏移读取校验。
  - 下载前后读取 D226/D227，用于诊断记录。
- 会话建立有 120 秒看门狗；用户取消时用 `shutdown()` 打断阻塞读取，避免 `fuji_setup` 一直等待。
- 不执行上游 `lib/` 之外的任何测试程序或脚本。仓库里可能包含写入、删除行为的测试套件，均**没有运行**。

## 更新上游的流程

1. 修改 `vendor-sync.sh` 中的 `LIBFUJI_SHA` / `LIBPICT_SHA`，并同步更新 `CapabilityCatalog.libfujiSHA` / `libpictSHA`。
2. 运行 `ios/scripts/vendor-sync.sh`。补丁无法应用时，先用 `--pristine DIR` 导出未打补丁的副本，再重做补丁。
3. 运行 `ios/scripts/c-core-check.sh`、`ios/scripts/vcam-integration.sh` 和 FujiKit 的 `swift test`。`c-core-check.sh` 把 Xcode 会报告的 `-Wshorten-64-to-32` 和 `-Wunsequenced` 当作错误，新版本上游如果引入这类问题，会在这里先失败。
4. 提交 `upstream/`、`VENDOR_MANIFEST.txt` 和补丁，然后用 `--check` 确认结果可以复现。
