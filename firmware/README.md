# firmware（ESP32 固件）

ESP-IDF v5.x 工程，A 模块：TCP 雷达接收 + 防抖滤波。

## 功能
- 连接 WiFi（Station）
- 起 TCP 服务端，监听 **8080**
- 雷达作为 TCP 客户端连入，持续发送单字符 `0`（无人）/ `1`（有人）
- 防抖滤波：连续 3 次相同才翻转状态
- 输出全局变量 `human_present`（1=有人 / 0=无人），供 B 模块读取

## 数据流
```
雷达 --TCP(8080)--> tcp_radar.c --feed--> human_state.c(防抖) --> human_present
                                                              └--> B 模块读取
```

## 编译前准备
1. 安装 ESP-IDF v5.x，加载环境变量（`get_idf`）
2. 设置目标芯片：
   ```bash
   idf.py set-target esp32
   ```
3. 改 WiFi：`main/wifi_sta.c` 顶部 `WIFI_SSID` / `WIFI_PASS`
4. 编译烧录：
   ```bash
   idf.py build flash monitor
   ```

## 自测（无雷达也能跑）
用网络调试助手 / `nc` 连 ESP32 的 IP:8080，发送 `0` 或 `1`：
```bash
# 终端示例（需 nc）
printf '1' | nc <ESP32_IP> 8080
```
观察串口日志 `human_present` 是否稳定切换（连续 3 次一致才变）。

## 文件
| 文件 | 职责 |
|------|------|
| `main/main.c` | 入口，串联各模块 |
| `main/wifi_sta.c/.h` | WiFi Station 连接 |
| `main/tcp_radar.c/.h` | TCP 8080 服务端，收 0/1 |
| `main/human_state.c/.h` | 防抖滤波 + `human_present` 全局变量 |
