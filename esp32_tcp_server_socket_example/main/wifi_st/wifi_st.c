/* Module: wi-fi (station) */

/* Includes */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi_st.h"

/* Includes - modules */
#include "../nvs_rw/nvs_rw.h"
#include "../socket_tcp_server/socket_tcp_server.h"

/* Defines - debug */
#define WIFI_TAG                "WIFI"

/* Static variables */
static bool is_ESP32_connected_to_wifi = false;

/* Funções locais */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static bool init_wifi_station(uint8_t * pt_ssid, uint8_t * pt_pass);

/* Function: informs wifi connection status
 * Params: none
 * Return: true: wi-fi connected
 *         false: wi-fi not connected
*/
bool get_status_wifi(void)
{
    return is_ESP32_connected_to_wifi;
}

/* Function: wi-fi event callback
 * Params: event arguments and data
 * Return: none
*/
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        ESP_LOGI(WIFI_TAG, "Conecting to wi-fi network...");
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        is_ESP32_connected_to_wifi = false;
        ESP_LOGI(WIFI_TAG, "Wi-fi connection has been terminated. Reconnecting to wi-fi network...");
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        is_ESP32_connected_to_wifi = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "Wi-fi connection is established. IP:" IPSTR, IP2STR(&event->ip_info.ip));
        tcp_socket_server_init();
    }
    else if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(WIFI_TAG, "station "MACSTR" conectado, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(WIFI_TAG, "station "MACSTR" desconectado, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

/* Function: init wifi
 * Params: none
 * Return: none
*/
void wifi_init_st(void)
{
    uint8_t wifi_SSID_loaded[MAX_SSID_ST_SIZE_WIFI] = {0};
    uint8_t wifi_pass_loaded[MAX_PASS_ST_SIZE_WIFI] = {0};
    size_t str_size = 0;
    bool init_wifi_station_status = true;
    
    /* Load wi-fi credentials from NVS */
    str_size = sizeof(wifi_SSID_loaded);   
    if (read_string_nvs(KEY_SSID_WIFI, (char *)wifi_SSID_loaded, str_size) == ESP_OK)
    {
        ESP_LOGI(WIFI_TAG, "SSID successfully read");
    }
    else
    {
        snprintf((char *)wifi_SSID_loaded, sizeof(wifi_SSID_loaded), "%s", WIFI_SSID_ST_DEFAULT);
        ESP_LOGI(WIFI_TAG, "Fail to read SSID. Default SSID: %s", wifi_SSID_loaded);
    }

    str_size = sizeof(wifi_pass_loaded);   
    if (read_string_nvs(KEY_PASS_WIFI, (char *)wifi_pass_loaded, str_size) == ESP_OK)
    {
        ESP_LOGI(WIFI_TAG, "Wifi password successfully read");
    }
    else
    {
        snprintf((char *)wifi_pass_loaded, sizeof(wifi_pass_loaded), "%s", WIFI_PASS_ST_DEFAULT);
        ESP_LOGI(WIFI_TAG, "Fail to read wi-fi password. Default password: %s", wifi_pass_loaded);
    }

    /* Init wi-fi */
    is_ESP32_connected_to_wifi = false;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    /* Set static IP */
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    esp_netif_dhcpc_stop(sta_netif);
    esp_netif_ip_info_t ip_fixo;
    ip_fixo.ip.addr = ipaddr_addr(WIFI_ST_STATIC_IP);
    ip_fixo.gw.addr = ipaddr_addr(WIFI_ST_GATEWAY);
    ip_fixo.netmask.addr = ipaddr_addr(WIFI_ST_NETWORK_MASK);
    esp_netif_set_ip_info(sta_netif, &ip_fixo);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /*  Register wi-fi event hadler callbacks */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL)); 
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    init_wifi_station_status = init_wifi_station(wifi_SSID_loaded, wifi_pass_loaded);

    if ( init_wifi_station_status == true)
    {
        ESP_LOGI(WIFI_TAG, "Wi-fi OK (station)");   
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "Fail to init wi-fi (station)");   
    }
}


/* Function: init wifi (st)
 * Params: none
 * Return: true: success
 *         false: fail
*/
static bool init_wifi_station(uint8_t * pt_ssid, uint8_t * pt_pass)
{
    bool status_wifi_st = false;

    wifi_config_t wifi_config = {0};
    snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s", (char *)pt_ssid);
    snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password), "%s", (char *)pt_pass);
    
    esp_err_t ret_esp_set_mode = esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t ret_esp_wifi_set_config = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_connect();
    
    if ( (ret_esp_set_mode == ESP_OK) && (ret_esp_wifi_set_config == ESP_OK) )
    {
        status_wifi_st = true;
    }
    else
    {
        status_wifi_st = false;
    }
      
    if (status_wifi_st == true)
    {
        ESP_LOGI(WIFI_TAG, "Wi-fi station OK\n");
    }
    else
    {
        ESP_LOGE(WIFI_TAG, "Fail to init wi-fi station");
    }

    return status_wifi_st;
}
