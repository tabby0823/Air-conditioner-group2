#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "human_state.h"

static const char *TAG = "TCP_RADAR";

#define RADAR_PORT 8080
#define RADAR_TASK_STACK 4096

static void radar_server_task(void *arg)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        ESP_LOGE(TAG, "socket failed");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(RADAR_PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        ESP_LOGE(TAG, "bind failed");
        close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    listen(listen_fd, 1);
    ESP_LOGI(TAG, "TCP server listening on port %d", RADAR_PORT);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        ESP_LOGI(TAG, "radar connected");

        char c;
        while (recv(client_fd, &c, 1, 0) == 1) {
            if (c == '0') {
                human_state_feed_raw(0);
            } else if (c == '1') {
                human_state_feed_raw(1);
            }
            /* 其它字符忽略 */
        }

        close(client_fd);
        ESP_LOGI(TAG, "radar disconnected");
    }
}

void tcp_radar_start(void)
{
    xTaskCreate(radar_server_task, "radar_server", RADAR_TASK_STACK, NULL, 5, NULL);
}
