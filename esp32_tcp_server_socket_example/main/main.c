#include <stdio.h>
#include <esp_task_wdt.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

/* Includes de outros módulos */
#include "nvs_rw/nvs_rw.h"
#include "wifi_st/wifi_st.h"
#include "breathing_light/breathing_light.h"

/* Define - debug */
#define APP_MAIN_DEBUG_TAG      "APP_MAIN"

/* Define - WDT time */
#define  WDT_TIME_PROJECT       60 //s

void app_main(void)
{
    esp_task_wdt_init(WDT_TIME_PROJECT, true);
    
    /* Init all modules (NVS, breathng light and wi-fi station) */
    init_nvs();
    init_breathing_light();
    wifi_init_st();

    /* From this point on, TCP socket server task works */
}
