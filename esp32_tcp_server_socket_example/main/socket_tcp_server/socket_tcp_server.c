/* Module: socket tcp server */

/* Includes */
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
#include "lwip/sockets.h"
#include <lwip/netdb.h>
#include <esp_task_wdt.h>

/* Includes - modules */
#include "../wifi_st/wifi_st.h"
#include "../socket_tcp_server/socket_tcp_server.h"

/* Tasks parametrization */
#include "../prio_tasks.h"
#include "../stacks_sizes.h"

/* Defines - task aguments and CPU */
#define PARAMS_TCP_SOCKET_SERVER           NULL
#define CPU_TCP_SOCKET_SERVER              1

/* Defines - debug */
#define SOCKET_TCP_SERVER_TAG "SOCKET_TCP_SERVER"

/* Static variables */
static int listen_sock = 0;
static struct sockaddr_storage source_addr;
static char addr_str[128] = {0};
static int sock = 0;
static char socket_tcp_rx_buffer[WIFI_SOCKET_TCP_SERVER_RECV_BUFFER_SIZE] = {0};
static char socket_tcp_tx_buffer[WIFI_SOCKET_TCP_SERVER_RECV_BUFFER_SIZE+20] = {0};

/* Socket task handler */
TaskHandle_t socket_task_handler;

/* Tasks */
static void tcp_socket_server_task(void *arg);

/* Function: init TCP socket server
 * Params: none
 * Return: none
 */
void tcp_socket_server_init(void)
{
    xTaskCreatePinnedToCore(tcp_socket_server_task, "tcp_socket_server_task",
                            SOCKET_TCP_TAM_TASK_STACK,
                            PARAMS_TCP_SOCKET_SERVER,
                            PRIO_TASK_SOCKET_TCP,
                            &socket_task_handler,
                            CPU_TCP_SOCKET_SERVER);
}

/* Function: TCP socket server task
 * Params: task arguments
 * Return: none
 */
static void tcp_socket_server_task(void *arg)
{
    int addr_family = AF_INET;
    int ip_protocol = 0;
    struct sockaddr_storage dest_addr;
    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    int recv_bytes_counter = 0;
    int keep_alive = 1;
    int keep_alive_idle_time = WIFI_SOCKET_TCP_SERVER_KEEPALIVE_IDLE;
    int keep_alive_time_interval = WIFI_SOCKET_TCP_SERVER_KEEPALIVE_INTERVAL;
    int keep_alive_attempts = WIFI_SOCKET_TCP_SERVER_KEEPALIVE_COUNT;
    socklen_t addr_len = sizeof(source_addr);
    bool is_there_any_client_connected = false;

    esp_task_wdt_add(NULL);

    /* Wait for wi-fi connection */
    while (get_status_wifi() == false)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    /* Configures IPv4 */
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(WIFI_PORT_SOCKET_TCP_SERVER);
    ip_protocol = IPPROTO_IP;

    /* Create TCP socket server */
    listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(SOCKET_TCP_SERVER_TAG, "Error: impossible to create TCP socket server. Error code: %d", errno);
        vTaskDelete(socket_task_handler);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Make socket bind */
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(SOCKET_TCP_SERVER_TAG, "Error: impossible to bind TCP socket server. Error code: %d", errno);
        ESP_LOGE(SOCKET_TCP_SERVER_TAG, "IPPROTO: %d", addr_family);
        terminate_TCP_socket_server();
    }

    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(SOCKET_TCP_SERVER_TAG, "Error: impossible to enter in the listen state. Error code: %d", errno);
        terminate_TCP_socket_server();
    }

    /* Configure TCP socket server to work in non-blocking mode */
    int flags = fcntl(listen_sock, F_GETFL);
    fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK);
    is_there_any_client_connected = false;

    while (1)
    {
        esp_task_wdt_reset();

        /* Proceed if a TCP socket client connects */
        if (is_there_any_client_connected == false)
        {
            sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

            if (sock >= 0)
            {
                /* There's TCP socket client connected. Configure Keep-Alive */
                setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(int));
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keep_alive_idle_time, sizeof(int));
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keep_alive_time_interval, sizeof(int));
                setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keep_alive_attempts, sizeof(int));
                int flags_client = fcntl(sock, F_GETFL);
                fcntl(sock, F_SETFL, flags_client | O_NONBLOCK);

                if (source_addr.ss_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                }

                is_there_any_client_connected = true;
                ESP_LOGI(SOCKET_TCP_SERVER_TAG, "TCP socket client IP: %s", addr_str);                
            }
            else
            {
                is_there_any_client_connected = false;
            }

            if (is_there_any_client_connected == false)
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                continue;
            }
        }

        /* Check for incoming bytes. If there are bytes to receive, echo them back to TCP socket client */
        memset(socket_tcp_rx_buffer, 0x00, sizeof(socket_tcp_rx_buffer));
        recv_bytes_counter = recv(sock, socket_tcp_rx_buffer, sizeof(socket_tcp_rx_buffer) - 1, MSG_DONTWAIT);
        
        if (recv_bytes_counter > 0)
        {
            memset(socket_tcp_tx_buffer, 0x00, sizeof(socket_tcp_tx_buffer));
            snprintf(socket_tcp_tx_buffer, sizeof(socket_tcp_tx_buffer), "\n\rReceived: %s", socket_tcp_rx_buffer);
            send(sock, socket_tcp_tx_buffer, strlen(socket_tcp_tx_buffer), 0);
            ESP_LOGI(SOCKET_TCP_SERVER_TAG, "%d bytes received from TCP socket client. Echoing them back to client...", recv_bytes_counter);
            
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/* Function: terminate TCP socket server
 * Params: none
 * Return: none
 */
void terminate_TCP_socket_server(void)
{
    close(listen_sock);
    vTaskDelete(socket_task_handler);
}