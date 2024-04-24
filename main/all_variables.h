#ifndef ALL_VARIABLES_H
#define ALL_VARIABLES_H

#include "sdkconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include "inttypes.h"
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/twai.h"
#include "driver/spi_common.h"

#include "esp_log.h"

#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "sdmmc_cmd.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sta_list.h"

#include "esp_sleep.h"
#include "time.h"
#include "esp_err.h"

#include "freertos/event_groups.h"
#include "lwip/apps/sntp.h" 
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "hal/twai_types.h"

#include "mdns.h"
#include "netdb.h"
#include <esp_ota_ops.h>

#define HB GPIO_NUM_8
#define SD GPIO_NUM_15
#define BK GPIO_NUM_16
#define WF GPIO_NUM_17

extern char strftime_buf0[64]; // char strftime_buf1[64];
extern int s_retry_num;
extern const int retry_count;
struct ntp_time {
    time_t now0;
};
typedef struct ntp_time ntp_time;

// extern unsigned char a[258];
// extern int length1;
// extern unsigned char b[258];
// extern int length2;
// extern unsigned char c[258];
// extern int length3;

extern unsigned char ac[138];
extern int length1;
extern unsigned char bc[138];
extern int length2;
extern unsigned char cc[138];
extern int length3;

extern unsigned char data7; 
extern char line[64];
extern unsigned char value[6];
extern char pass[64];
extern double diff;
extern int fl;
struct all_handle {
    nvs_handle_t value_r;
    nvs_handle_t value_ab;
    nvs_handle_t value_mid;
    nvs_handle_t time_ani;
    nvs_handle_t time_ab;
    nvs_handle_t time_mid;
    nvs_handle_t time_mid_opn;
    nvs_handle_t time_mid_cls;
    nvs_handle_t rec;
    nvs_handle_t rec_tm;
    nvs_handle_t wifi_s;
    nvs_handle_t wifi_p;
    nvs_handle_t rec_s;
    nvs_handle_t rec_p;
    nvs_handle_t c_value_opn;
    nvs_handle_t c_value_opn1;
    nvs_handle_t c_value_cls;
    nvs_handle_t c_value_cls1;
    nvs_handle_t rbt_esp;
};
typedef struct all_handle all_handle;
extern int time1[2];
extern char pswd[10];
extern bool can_flag;
extern bool wf_flag;

#endif