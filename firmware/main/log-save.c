#include "log-save.h"

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "human_state.h"
#include "nvs.h"

#define STORE_VERSION 1U
#define STORE_NAMESPACE "detect_log"
#define STORE_KEY "history"

static const char *TAG = "DETECTION_STORE";
typedef struct {
    uint32_t version;
    uint32_t count;
    uint32_t next;
    detection_record_t records[DETECTION_STORE_CAPACITY];
} store_blob_t;

static store_blob_t s_data;
static SemaphoreHandle_t s_mutex;
static TaskHandle_t s_log_task;

static void clear_history(void)
{
    memset(&s_data, 0, sizeof(s_data));
    s_data.version = STORE_VERSION;
}

static esp_err_t persist_history(void)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(handle, STORE_KEY, &s_data, sizeof(s_data));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static int valid_timestamp(const char* ts)
{
    if (ts == NULL || strnlen(ts, 20) != 19) {
        return 0;
    }
    for (size_t i = 0; i < 19; ++i) {
        char expected = (i == 4 || i == 7) ? '-' :
            (i == 10) ? 'T' :
            (i == 13 || i == 16) ? ':' : 0;
        if (expected ? ts[i] != expected : ts[i] < '0' || ts[i] > '9') {
            return 0;
        }
    }
    return 1;
}

esp_err_t detection_store_init(void)
{
    if (s_mutex != NULL) {
        return ESP_OK;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORE_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        clear_history();
    }
    else if (err != ESP_OK) {
        return err;
    }
    else {
        size_t size = sizeof(s_data);
        err = nvs_get_blob(handle, STORE_KEY, &s_data, &size);
        nvs_close(handle);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            clear_history();
        }
        else if (err != ESP_OK && err != ESP_ERR_NVS_INVALID_LENGTH) {
            return err;
        }
        else if (err == ESP_ERR_NVS_INVALID_LENGTH || size != sizeof(s_data) ||
            s_data.version != STORE_VERSION ||
            s_data.count > DETECTION_STORE_CAPACITY ||
            s_data.next >= DETECTION_STORE_CAPACITY) {
            ESP_LOGW(TAG, "Invalid stored history; replacing it with an empty log");
            clear_history();
            err = persist_history();
            if (err != ESP_OK) {
                return err;
            }
        }
    }

    s_mutex = xSemaphoreCreateMutex();
    return s_mutex != NULL ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t detection_store_append(int state, const char* ts)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((state != 0 && state != 1) || !valid_timestamp(ts)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    store_blob_t next = s_data;
    detection_record_t* record = &next.records[next.next];
    memcpy(record->ts, ts, sizeof(record->ts));
    record->human_present = (uint8_t)state;
    next.next = (next.next + 1) % DETECTION_STORE_CAPACITY;
    if (next.count < DETECTION_STORE_CAPACITY) {
        ++next.count;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORE_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(handle, STORE_KEY, &next, sizeof(next));
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }
        nvs_close(handle);
    }
    if (err == ESP_OK) {
        s_data = next;
    }
    xSemaphoreGive(s_mutex);
    return err;
}

esp_err_t detection_store_append_now(int state)
{
    time_t now = time(NULL);
    struct tm local_time;
    char ts[20];
    if (localtime_r(&now, &local_time) == NULL ||
        local_time.tm_year + 1900 < 2020 ||
        strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", &local_time) != 19) {
        return ESP_ERR_INVALID_STATE;
    }
    return detection_store_append(state, ts);
}

esp_err_t detection_store_recent(size_t limit, detection_record_t* out,
    size_t* written)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (written == NULL || limit > DETECTION_STORE_CAPACITY ||
        (limit > 0 && out == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    size_t n = limit < s_data.count ? limit : s_data.count;
    for (size_t i = 0; i < n; ++i) {
        size_t index = (s_data.next + DETECTION_STORE_CAPACITY - 1 - i) %
            DETECTION_STORE_CAPACITY;
        out[i] = s_data.records[index];
    }
    *written = n;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static void log_task(void *arg)
{
    (void)arg;
    esp_err_t err = detection_store_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "History initialization failed: %s", esp_err_to_name(err));
        s_log_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    esp_netif_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SNTP start failed: %s", esp_err_to_name(err));
    }

    int last = -1;
    esp_err_t last_error = ESP_OK;
    while (1) {
        int current = human_present;
        if ((current == 0 || current == 1) && current != last) {
            err = detection_store_append_now(current);
            if (err == ESP_OK) {
                last = current;
                last_error = ESP_OK;
            }
            else if (err != last_error) {
                ESP_LOGW(TAG, "State not saved yet: %s", esp_err_to_name(err));
                last_error = err;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void detection_store_task_start(void)
{
    if (s_log_task != NULL) {
        return;
    }
    if (xTaskCreate(log_task, "log_task", 4096, NULL, 4, &s_log_task) != pdPASS) {
        ESP_LOGE(TAG, "Unable to create log task");
    }
}
