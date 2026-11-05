#pragma once
#include <stdint.h>

/**
 * 人员状态全局变量。
 * 1 = 有人，0 = 无人。
 * 由 A 模块（TCP 雷达 + 防抖）维护，B 模块直接读取。
 * 用 volatile 防止编译器优化（可能被中断/其他任务读取）。
 */
extern volatile int human_present;

/**
 * 喂入雷达上报的原始值（0 或 1），内部做防抖滤波后更新 human_present。
 * 连续 DEBOUNCE_COUNT 次相同才翻转，避免雷达抖动误判。
 */
void human_state_feed_raw(int raw);
