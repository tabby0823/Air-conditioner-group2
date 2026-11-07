#pragma once
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"

#define STORE_VERSION 1U
#define STORE_NAMESPACE "detect_log"
#define STORE_KEY "history"
#define DETECTION_STORE_CAPACITY 50


typedef struct 
{
    char ts[20];             /* YYYY-MM-DDTHH:MM:SS, UTF-8/ASCII */
    uint8_t human_present;   /* 0 or 1 */
} detection_record_t;

/* Call once after nvs_flash_init(). Existing records are loaded from flash. */
esp_err_t detection_store_init(void);

/* Persist one debounced state change. The caller supplies a synchronized timestamp. */
esp_err_t detection_store_append(int human_present, const char* ts);

/* Use the ESP32 system clock; returns ESP_ERR_INVALID_STATE before time sync. */
esp_err_t detection_store_append_now(int human_present);

/* Copy up to limit records, newest first. out must have room for limit records. */
esp_err_t detection_store_recent(size_t limit, detection_record_t* out,
    size_t* written);
