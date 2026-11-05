#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_sta.h"
#include "tcp_radar.h"
#include "human_state.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    /* 1. 连 WiFi */
    wifi_sta_init();

    /* 2. 启动 TCP 雷达服务端（端口 8080，收 0/1） */
    tcp_radar_start();

    /* 3. 主循环：这里只是演示打印 human_present
     *    B 模块后续直接读取全局变量 human_present 即可控制空调 */
    while (1) {
        ESP_LOGI(TAG, "human_present = %d", human_present);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
