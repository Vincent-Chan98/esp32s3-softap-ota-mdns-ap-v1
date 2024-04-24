#ifndef SD_CARD_H
#define SD_CARD_H

#include "esp_err.h"
#include "all_variables.h"

// void fileStatus_init(); 
void sdcard(); // Function prototype
esp_err_t fileRead(void);

bool is_our_netif(const char *prefix, esp_netif_t *netif);
void start(void);
void stop(void);
void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void on_got_ipv6(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void on_wifi_connect(void *esp_netif, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t readWifi(void);
esp_netif_t *get_example_netif_from_desc(const char *desc);
void wifi_stop(void);
esp_err_t wifi(void);

esp_err_t softap_init(void);
#endif
