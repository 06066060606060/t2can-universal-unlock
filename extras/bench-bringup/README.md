# T2CAN-Bringup · T-2CAN V1.0 板级自检固件

只读、listen-only 的台架固件，用来在上车/开功能前确认：

- CAN A（MCP2515，P2）能否在 500 kbit/s 收到帧；
- CAN B（ESP32 TWAI，P1）能否在 500 kbit/s 收到帧；
- 板子 Wi-Fi、USB、供电是否正常。

**它不会发送任何 CAN 帧，也不会 ACK 总线**，因此适合在隔离台架上做首次验证。

## 构建

```sh
cd extras/bench-bringup/T2CAN-Bringup
arduino-cli compile \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc,FlashMode=qio,CPUFreq=240,DebugLevel=none" \
  --output-dir dist .
```

需要 ESP32 core 3.3.12 + autowp-mcp2515（与主固件相同）。

## 烧录与使用

- 用 USB 整片烧录（或 OTA，如果当前分区兼容）：

```sh
esptool.py --chip esp32s3 --port COM3 write_flash -z 0x0 \
  dist/T2CAN-Bringup.ino.merged.bin
```

- 上电后连接热点 **`T2CAN-BRINGUP` / `12345678`**，打开 `http://192.168.4.1`。
- 页面显示两路初始化状态、收帧计数和最近 16 帧原始数据；串口 115200 同时打印 `A/B 0xID dlc data`。

## 判断方法

1. 只接 CAN A（P2）到分析仪/台架信号源：页面 CAN A 计数应增长。
2. 只接 CAN B（P1）：页面 CAN B 计数应增长。
3. 若某一路始终为 0：
   - 检查 CANH/CANL、SGND/DGND 接线；
   - 检查总线两端 120 Ω 终端（含板上 RZ1/RZ2）；
   - 如果只是 CAN B 无帧，可核对原理图 GPIO6/7 的 `TXB_CAN/RXB_CAN` 标注与官方 TX=7/RX=6 是否相反（本固件按官方 7/6）。

> 本固件仅用于硬件/总线验证，不包含任何车辆控制逻辑。
