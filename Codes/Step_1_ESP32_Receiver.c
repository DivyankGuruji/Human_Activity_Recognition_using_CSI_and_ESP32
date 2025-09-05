#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/sockets.h"

#define WIFI_SSID "NETGEAR08"
#define WIFI_PASS "PASSWORD4"
#define UDP_PORT 5005
#define DEST_IP "192.168.1.2"

static int udp_socket;
static struct sockaddr_in dest_addr;
static bool is_connected = false;

static void wifi_csi_cb(void *ctx, wifi_csi_info_t *info) 
{
    if (!info || !info->buf || info->len == 0 || !is_connected)
    {
        return;
    }

    int8_t *csi_data = info->buf;
    int len = info->len;

    char msg[2048];
    int offset = 0;

    for (int i = 0; i < len && (i / 2) < 64; i += 2) 
    {
        int8_t real = csi_data[i];
        int8_t imag = csi_data[i + 1];

        float amplitude = sqrtf(real * real + imag * imag);
        float phase_rad = atan2f((float)imag, (float)real);
        offset = offset + snprintf(msg + offset, sizeof(msg) - offset, "%.2f,%.2f ", amplitude, phase_rad);
    }

    if (offset > 0) msg[offset - 1] = '\0';
    sendto(udp_socket, msg, strlen(msg), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) 
{
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        is_connected = false;
        esp_wifi_connect();
    } 
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        is_connected = true;
    }
}

void init_udp_socket() 
{
    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(udp_socket < 0) 
    {
        return;
    }
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);
}

void wifi_init_sta() 
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);
    init_udp_socket();

    wifi_csi_config_t csi_config = {
        .lltf_en = true,
        .htltf_en = true,
        .stbc_htltf2_en = true,
        .ltf_merge_en = true,
        .channel_filter_en = false,
        .manu_scale = false,
        .shift = false,
    };

    esp_wifi_set_csi_config(&csi_config);
    esp_wifi_set_csi_rx_cb(wifi_csi_cb, NULL);
    esp_wifi_set_csi(true);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(11, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

void app_main(void) 
{
    nvs_flash_init();
    wifi_init_sta();
}