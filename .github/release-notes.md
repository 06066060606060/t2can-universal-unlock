# T2CAN Universal v3.29.1 · 中文 Dashboard

本 Release 由 GitHub Actions 自动编译，基于上游
[06066060606060/t2can-universal-unlock](https://github.com/06066060606060/t2can-universal-unlock)
v3.29.1，仅增加 Dashboard 中文化与构建/发布自动化，未修改 CAN 注入策略。

## 下载

| 文件 | 用途 |
| --- | --- |
| `T2CAN-Universal-v3.29.1-LP_YL-zh.OTA.bin` | 网页 OTA 镜像（设备需已有 Universal 的 3 MB app 分区） |
| `T2CAN-Universal-v3.29.1-LP_YL-zh.fullflash.bin` | 16 MB 整片镜像（USB 0x0 烧录；从旧固件切换时使用） |
| `SHA256SUMS.txt` | 上述文件校验和 |

## 烧录

从旧固件（如 `t2can-monitor`，1.25 MB app 分区）切换时 **不能直接 OTA**，请用整片镜像：

```sh
esptool.py --chip esp32s3 --port COM3 write_flash -z 0x0 \
  T2CAN-Universal-v3.29.1-LP_YL-zh.fullflash.bin
```

整片烧录会覆盖 bootloader、分区表和 NVS，请先备份。已在 Universal 上的设备可以直接在网页“固件更新”里上传 OTA 镜像。

## 首次启动

- 默认热点 `T2CAN-****`，密码 `12345678`，地址 `http://192.168.4.1`。
- 首次启动必须选择车型、CAN 拓扑和转向灯类型；保存重启后 CAN 功能才启用。
- 默认接线：CAN A = Body，CAN B = Chassis；Model YL / Model S/X 按上游 README 反接。

## 中文 Dashboard

- 静态文案、按钮、设置项、状态与可见属性已中文化；`html lang` 修正为 `zh-CN`。
- JS 动态写入的状态/计数/提示由页面内嵌中文词典实时翻译，只影响显示。
- 技术术语保留英文：CAN / NAG / TLSSC / R79 / AP / NOA / MUX1 / HO / TX / RX / 0x 帧 ID 等。
- 截图见仓库 README 与 `docs/images/zh/`（模拟数据，未连接车辆）。

## 注意

- 实验性研究固件，未经过台架/实车验证；**不要用于公共道路**。
- 高风险功能（NAG 扭矩、R79/召唤、TLSSC Restore 等）默认关闭，请先在隔离台架验证。
- GPL-3.0；分发固件请同时提供对应源码。
