#pragma once

/**
 * 初始化 WiFi Station 并连接到 AP，阻塞直到拿到 IP 或重试耗尽。
 * SSID / 密码在 wifi_sta.c 顶部的 WIFI_SSID / WIFI_PASS 宏里改。
 */
void wifi_sta_init(void);
