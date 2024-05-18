/* Module: breathing light */

/* Includes */
#include <string.h>
#include <esp_task_wdt.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

/* Tasks parametrization */
#include "../prio_tasks.h"
#include "../stacks_sizes.h"

/* Defines - debug */
#define BREATHING_LIGHT_TAG                "BREATHING_LIGHT"

/* Defines - On/Off breathng light LED time */
#define ON_OFF_LED_TIME                  500 //ms

/* Defines - breathing light LED GPIO */
#define GPIO_BREATHING_LIGHT_LED                  17
#define GPIO_BREATHING_LIGHT_LED_OUTPUT_PIN_SEL   (1ULL << GPIO_BREATHING_LIGHT_LED)

/* Defines - task aguments and CPU */
#define ARGS_BREATHING_LIGHT_TASK          NULL
#define CPU_BREATHING_LIGHT                1

/* TCP socket server task handler */
TaskHandle_t handler_breathing_light;

/* Task prototype */
static void breathing_light_task(void *arg);

/* Function: init breathing light
 * Params: none
 * Return: none
 */
void init_breathing_light(void)
{
    gpio_config_t io_conf = {};

    /* Inicializa GPIO */
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_BREATHING_LIGHT_LED_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    xTaskCreatePinnedToCore(breathing_light_task, "breathing_light_task",
                            BREATHING_LIGHT_TAM_TASK_STACK,
                            ARGS_BREATHING_LIGHT_TASK,
                            PRIO_TASK_BREATHING_LIGHT,
                            &handler_breathing_light,
                            CPU_BREATHING_LIGHT);
}

/* Function: breathing light task
 * Params: task's arguments
 * Return: none
 */
static void breathing_light_task(void *arg)
{ 
    esp_task_wdt_add(NULL);

    while (1)
    {
        esp_task_wdt_reset();
        gpio_set_level(GPIO_BREATHING_LIGHT_LED, 1);
        vTaskDelay(ON_OFF_LED_TIME / portTICK_PERIOD_MS);

        esp_task_wdt_reset();        
        gpio_set_level(GPIO_BREATHING_LIGHT_LED, 0);
        vTaskDelay(ON_OFF_LED_TIME / portTICK_PERIOD_MS);
    }
}