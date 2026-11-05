#include "human_state.h"
#include "esp_log.h"

static const char *TAG = "HUMAN_STATE";

volatile int human_present = 0;

/* 防抖阈值：连续多少次相同才认定状态稳定 */
#ifndef DEBOUNCE_COUNT
#define DEBOUNCE_COUNT 3
#endif

static int s_last_raw = 0;     /* 最近一次“方向” */
static int s_stable_cnt = 0;   /* 当前方向连续计数 */

void human_state_feed_raw(int raw)
{
    if (raw != 0 && raw != 1) {
        return; /* 只接受 0 / 1 */
    }

    if (raw == s_last_raw) {
        s_stable_cnt++;
        if (s_stable_cnt >= DEBOUNCE_COUNT && human_present != raw) {
            human_present = raw;
            ESP_LOGI(TAG, "human_present -> %d", human_present);
        }
    } else {
        /* 方向变化，重新开始计数，不立即翻转，等稳定 */
        s_last_raw = raw;
        s_stable_cnt = 1;
    }
}
