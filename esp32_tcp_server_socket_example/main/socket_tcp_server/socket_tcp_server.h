/* Header file: socket tcp server */

#ifndef HEADER_MOD_SOCKET_TCP_OTA_SERVER_WIFI
#define HEADER_MOD_SOCKET_TCP_OTA_SERVER_WIFI

/* Defines: TCP socket server params */
#define WIFI_PORT_SOCKET_TCP_SERVER                  5000
#define WIFI_SOCKET_TCP_SERVER_KEEPALIVE_IDLE        5
#define WIFI_SOCKET_TCP_SERVER_KEEPALIVE_INTERVAL    5
#define WIFI_SOCKET_TCP_SERVER_KEEPALIVE_COUNT       10
#define WIFI_SOCKET_TCP_SERVER_RECV_BUFFER_SIZE      1024

#endif

/* Prototypes */
void tcp_socket_server_init(void);
void terminate_TCP_socket_server(void);