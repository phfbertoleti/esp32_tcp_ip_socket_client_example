/* Module: NVS */

/* Includes */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_rw.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"

/* Uncomment the line below to force NVS clean in case of error to inti NVS */
#define FORCE_NVS_CLEAN_IN_CASE_OF_ERROR

/* Defines - debug */
#define NVS_TAG            "NVS"

/* Define - namespace */
#define NAMESPACE_NVS      "esp32s3"

/* Function: inicializa NVS
 * Params: none
 * Return: none
*/
void init_nvs(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();

    #ifdef FORCE_NVS_CLEAN_IN_CASE_OF_ERROR
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    #endif

    ESP_ERROR_CHECK( ret );                          
}

/* Function: store a string into NVS
 * Params: key and data pointers
 * Return: ESP_OK: success
 *         !ESP_OK: fail
*/
esp_err_t store_string_nvs(char * pt_key, char * pt_string)
{
    esp_err_t ret = ESP_FAIL;
    nvs_handle handler_part_nvs;

    if (pt_key == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: key pointer is null");
        ret = ESP_FAIL;
        goto END_NVS_STORE;
    }

    if (pt_string == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: string pointer is null");
        ret = ESP_FAIL;
        goto END_NVS_STORE;
    }
    
    ret = nvs_open(NAMESPACE_NVS, NVS_READWRITE, &handler_part_nvs);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Error: impossible to access NVS partition");
        goto END_NVS_STORE;  
    }

    ret = nvs_set_str(handler_part_nvs, pt_key, pt_string);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Error: fail to save string into NVS");
        goto END_NVS_STORE; 
    }

    ret = nvs_commit(handler_part_nvs);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Error: fail to commit data to NVS");
        goto END_NVS_STORE; 
    }

END_NVS_STORE:
    return ret;
}

/* Function: read string stored into NVS
 * Params: key, string pointers and string size
 * Return: ESP_OK: success
 *         !ESP_OK: fail
*/
esp_err_t read_string_nvs(char * pt_key, char * pt_string, size_t tam_str)
{
    esp_err_t ret = ESP_FAIL;
    nvs_handle handler_part_nvs;

    if (pt_key == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: key pointer is null");
        ret = ESP_FAIL;
        goto END_READ_NVS_STRING;
    }

    if (pt_string == NULL)
    {
        ESP_LOGE(NVS_TAG, "Error: string pointer is null");
        ret = ESP_FAIL;
        goto END_READ_NVS_STRING;
    }
    
    ret = nvs_open(NAMESPACE_NVS, NVS_READWRITE, &handler_part_nvs);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Error: impossible to access NVS partition");
        goto END_READ_NVS_STRING;  
    }

    ret = nvs_get_str(handler_part_nvs, pt_key, pt_string, &tam_str);

    if (ret != ESP_OK)
    {
        ESP_LOGE(NVS_TAG, "Error: fail to read string from NVS");
        goto END_READ_NVS_STRING;  
    }

END_READ_NVS_STRING:
    return ret;
}

/* Function: test NVS write and read string processes 
 * Params: none
 * Return: none
*/
esp_err_t test_nvs_write_and_read(void)
{
    esp_err_t ret = ESP_FAIL;
    char str_nvs_test[sizeof(TEST_NVS_STRING)+1] = {0};
    char str_key_nvs_test[sizeof(KEY_TEST_NVS)+1] = {0};
    
    memset(str_nvs_test, 0x00, sizeof(str_nvs_test));
    memset(str_key_nvs_test, 0x00, sizeof(str_key_nvs_test));
    snprintf(str_nvs_test, sizeof(str_nvs_test), "%s", TEST_NVS_STRING);
    snprintf(str_key_nvs_test, sizeof(str_key_nvs_test), "%s", KEY_TEST_NVS);

    if (store_string_nvs(str_key_nvs_test, str_nvs_test) == ESP_OK)
    {
        ESP_LOGI(NVS_TAG, "NVS test: write string test is ok");
        memset(str_nvs_test, 0x00, sizeof(str_nvs_test));
        
        if (read_string_nvs(str_key_nvs_test, str_nvs_test, sizeof(str_nvs_test)) == ESP_OK)
        {
            ESP_LOGI(NVS_TAG, "NVS test: read string test is ok");

            if (strcmp(str_nvs_test, TEST_NVS_STRING) == 0)
            {
                ESP_LOGI(NVS_TAG, "NVS test: strings (read and write) match: %s", str_nvs_test);
            }
            else
            {
                ESP_LOGE(NVS_TAG, "NVS test: strings (read and write) don't match");
            }
        }
        else
        {
            ESP_LOGE(NVS_TAG, "NVS test: fail to read data from NVS");
        }
    }
    else
    {
         ESP_LOGE(NVS_TAG, "NVS test: fail to write data to NVS");
    }    
    
    return ret;
}

/* Function: clean NVS data
 * Params: none
 * Return: ESP_OK: success
 *         !ESP_OK: fail
*/
esp_err_t clean_NVS_partition(void)
{
    esp_err_t ret;

    ret = nvs_flash_erase();

    if (ret != ESP_OK) 
    {
        ESP_LOGE(NVS_TAG, "Error: impossible to clean NVS partition\n");
    }
    else
    {
        ESP_LOGI(NVS_TAG, "NVS partition clean process is ok\n");
    }

    ESP_ERROR_CHECK( ret );    

    return ret;
}