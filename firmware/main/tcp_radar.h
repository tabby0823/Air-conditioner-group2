#pragma once

/**
 * 启动 TCP 雷达服务端任务。
 * 监听 8080，等待雷达作为 TCP 客户端连入，
 * 持续读取单字符 0/1 并喂给 human_state 防抖模块。
 */
void tcp_radar_start(void);
