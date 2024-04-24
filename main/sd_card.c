#include "sd_card.h"
#include "all_variables.h"
#include "heartbeat.h"

//----- SD CARD CONFIG-----//
#define MOUNT_POINT "/sdcard"
// #define PIN_NUM_MISO  GPIO_NUM_19    // EITA IoT PCB V0.3
#define PIN_NUM_MISO  GPIO_NUM_13       // EITA IoT PCB V0.4
#define PIN_NUM_MOSI  GPIO_NUM_18
#define PIN_NUM_CLK   GPIO_NUM_36
#define PIN_NUM_CS    GPIO_NUM_5
// #define PIN_NUM_CD 

//----- OTHER DEFINITIONS -----//
static const char *TAG1 = "sd_card";
char *lines[8]; //assuming a maximum 8 lines to be read from SD card
char w_conf[64];
char conf[32] = "";
char pas[64] = "";

sdmmc_card_t *card;  
esp_err_t res;

#define NO_FILE GPIO_NUM_15
#define N_FILE GPIO_NUM_16
FILE *configFile, *newFile, *pFile, *wFile;
// #define WIFI_SSID "Anonymous"
// #define WIFI_PASS "1a2b3c4d5e"
#define WIFI_SSID "ESP32S3_AP"
#define WIFI_PASS "123456789"
#define NR_OF_IP_ADDRESSES_TO_WAIT_FOR (s_active_interfaces*2)

static const char * TAG = "mdns-test";
esp_ip6_addr_t s_ipv6_addr;
esp_ip4_addr_t s_ip_addr;
xSemaphoreHandle s_semph_get_ip_addrs;
esp_netif_t *s_example_esp_netif = NULL;
int s_active_interfaces = 0;

esp_err_t readWifi(void);
void wifi_stop(void);

const char *s_ipv6_addr_types[] = {
    "ESP_IP6_ADDR_IS_UNKNOWN",
    "ESP_IP6_ADDR_IS_GLOBAL",
    "ESP_IP6_ADDR_IS_LINK_LOCAL",
    "ESP_IP6_ADDR_IS_SITE_LOCAL",
    "ESP_IP6_ADDR_IS_UNIQUE_LOCAL",
    "ESP_IP6_ADDR_IS_IPV4_MAPPED_IPV6"
};

void sdcard()
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG1, "Initializing SD card");ESP_LOGI(TAG1, "Using SPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    // host.max_freq_khz = 10000;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    res = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (res != ESP_OK) {ESP_LOGE(TAG1, "Failed to initialize bus.");return;}
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;
    // slot_config.gpio_cd = PIN_NUM_CD;
    // ESP_LOGI(TAG1, "Mounting filesystem");
    res = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (res != ESP_OK) {printf((res == ESP_FAIL)?"Failed to mount filesystem. ":"\nFailed to initialize the card, MAKE SURE SD CARD IS IN PLACE\n\n");return;}
    ESP_LOGI(TAG1, "Filesystem mounted");gpio_set_level(SD, 1);
}

esp_err_t fileRead(void)
{
    all_handle myHandle;
    int line_cnt = 0; /*bool flag = true;*/ bool flag1 = true;
    gpio_set_direction(BK, GPIO_MODE_OUTPUT);
    char alls_path[20];// = MOUNT_POINT"/alls.txt";
    char new_path[20];
    char pasw[15];char ans[] = "ABC8054DE";
    snprintf(new_path, sizeof(new_path), "%s%s", MOUNT_POINT,"/BAK.TXT");
    snprintf(alls_path, sizeof(alls_path), "%s%s",MOUNT_POINT, "/ALLS.TXT");
    snprintf(pasw, sizeof(pasw), "%s%s",MOUNT_POINT, "/P.TXT");
    configFile = fopen(alls_path, "r"); //--> read 
    newFile = fopen(new_path, "r");
    pFile = fopen(pasw, "r");
    
    if (pFile != NULL)
    {
        fgets(pswd, sizeof(pswd), pFile);
        if (strcmp(ans,pswd) == 0)
        {
            if (configFile == NULL && newFile == NULL)
            {
                for (; fl < 10; fl++)
                {
                    // ESP_LOGE(TAG1, "Failed to open file for reading");
                    vTaskDelay(1000/portTICK_PERIOD_MS);gpio_set_level(BK, 1);
                    vTaskDelay(1000/portTICK_PERIOD_MS);gpio_set_level(BK, 0);
                }                      
                fclose(configFile);gpio_set_level(BK, 0);
                newFile = fopen(new_path, "w");        
                fclose(newFile);gpio_set_level(BK, 1); // if newFile absent 
                esp_restart();
                // return ESP_OK;        
            }
            else
            {
                newFile = fopen(new_path, "w");
                if(configFile != NULL && flag1 == true)
                {
                    while (fgets(line, sizeof(line), configFile))
                    {
                        char *pos = strchr(line, '\n');
                        // strip newline
                        if (pos){*pos = '\0';}lines[line_cnt] = strdup(line);
                        if (lines[line_cnt] == NULL){ESP_LOGE(TAG1, "Memory allocation failed");break;}line_cnt++;
                        if (line_cnt >= 8) {break;}
                    }   
                    fclose(configFile);
                    for (uint8_t i = 0; i < 3; i++)
                    { 
                        nvs_open("value_s"+i, NVS_READWRITE,&myHandle.value_r);
                        value[i] = (unsigned char)strtol(lines[i],NULL,16);     
                        nvs_set_i32(myHandle.value_r, "abab"+i, (int32_t)value[i]);      
                        nvs_commit(myHandle.value_r);   
                        nvs_close(myHandle.value_r);
                    }    
                    for (int i = 0; i < 1; i++)
                    {
                        nvs_open("time_s"+i, NVS_READWRITE,&myHandle.time_ani);
                        time1[i] = atoi(lines[i+3]);           
                        nvs_set_i32(myHandle.time_ani, "ani"+i, (int32_t)time1[i]);
                        nvs_commit(myHandle.time_ani);   
                        nvs_close(myHandle.time_ani);
                    }     
                    for (uint8_t i = 3; i < 6; i++)
                    { 
                        nvs_open("value_s"+i, NVS_READWRITE,&myHandle.value_r);
                        value[i] = (unsigned char)strtol(lines[i+1],NULL,16);     
                        nvs_set_i32(myHandle.value_r, "abab"+i, (int32_t)value[i]);      
                        nvs_commit(myHandle.value_r);   
                        nvs_close(myHandle.value_r);
                    }    
                    for (int i = 1; i < 2; i++)
                    {
                        nvs_open("time_s"+i, NVS_READWRITE,&myHandle.time_ani);
                        time1[i] = atoi(lines[i+6]);           
                        nvs_set_i32(myHandle.time_ani, "ani"+i, (int32_t)time1[i]);
                        nvs_commit(myHandle.time_ani);   
                        nvs_close(myHandle.time_ani);
                    }        
                }
                else if (newFile != NULL)
                {
                    for (int i = 0; i < (sizeof(value)/sizeof(value[0]))-3; i++)
                    {
                        nvs_open("value_s"+i, NVS_READONLY, &myHandle.rec);
                        nvs_get_i32(myHandle.rec, "abab"+i, (int32_t*)&value[i]);
                        fprintf(newFile,"0x%02X\n",value[i]);
                        nvs_close(myHandle.rec);
                    }
                    for (int i = 0; i < (sizeof(time1)/sizeof(time1[0]))-1; i++)
                    {
                        nvs_open("time_s"+i, NVS_READONLY, &myHandle.rec_tm);
                        nvs_get_i32(myHandle.rec_tm, "ani"+i, (int32_t*)&time1[i]);
                        fprintf(newFile,"%d\n",time1[i]);
                        nvs_close(myHandle.rec_tm);
                    }
                    for (int i = 3; i < sizeof(value)/sizeof(value[0]); i++)
                    {
                        nvs_open("value_s"+i, NVS_READONLY, &myHandle.rec);
                        nvs_get_i32(myHandle.rec, "abab"+i, (int32_t*)&value[i]);
                        fprintf(newFile,"0x%02X\n",value[i]);
                        nvs_close(myHandle.rec);
                    }
                    for (int i = 1; i < sizeof(time1)/sizeof(time1[0]); i++)
                    {
                        nvs_open("time_s"+i, NVS_READONLY, &myHandle.rec_tm);
                        nvs_get_i32(myHandle.rec_tm, "ani"+i, (int32_t*)&time1[i]);
                        fprintf(newFile,"%d\n",time1[i]);
                        nvs_close(myHandle.rec_tm);
                    }     
                    fclose(newFile);
                }
            }
        }
        else
        {
            gpio_set_level(BK, 0);
            fclose(pFile);
            return ESP_FAIL;
        }
        fclose(pFile);
    }         
    else{                 
        all_handle mdHde;
        for (uint8_t i = 0; i < sizeof(value)/sizeof(value[0]); i++)
        {
            nvs_open("value_s"+i, NVS_READONLY, &mdHde.value_mid);
            nvs_get_i32(mdHde.value_mid, "abab"+i, (int32_t*)&value[i]);
            nvs_close(mdHde.value_mid);
        }
        for (int i = 0; i < sizeof(time1)/sizeof(time1[0]); i++)
        {
            nvs_open("time_s"+i, NVS_READONLY, &mdHde.time_mid);
            nvs_get_i32(mdHde.time_mid, "ani"+i, (int32_t*)&time1[i]);
            nvs_close(mdHde.time_mid);
        }                              
    }
    return ESP_OK; 
}



bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

// void start(void)
// {
//     s_example_esp_netif = readWifi();
//     s_active_interfaces++;
//     s_semph_get_ip_addrs = xSemaphoreCreateCounting(NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0);
// }

void stop(void){wifi_stop();s_active_interfaces--;}

void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    xSemaphoreGive(s_semph_get_ip_addrs);
}

void on_got_ipv6(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    if (!is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv6 from another netif: ignored");
        return;
    }
    esp_ip6_addr_type_t ipv6_type = esp_netif_ip6_get_addr_type(&event->ip6_info.ip);
    ESP_LOGI(TAG, "Got IPv6 event: Interface \"%s\" address: " IPV6STR ", type: %s", esp_netif_get_desc(event->esp_netif),
             IPV62STR(event->ip6_info.ip), s_ipv6_addr_types[ipv6_type]);
    if (ipv6_type == ESP_IP6_ADDR_IS_LINK_LOCAL) {
        memcpy(&s_ipv6_addr, &event->ip6_info.ip, sizeof(s_ipv6_addr));
        xSemaphoreGive(s_semph_get_ip_addrs);
    }
}

void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

void on_wifi_connect(void *esp_netif, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    esp_netif_create_ip6_linklocal(esp_netif);
}

esp_err_t readWifi(void)
{
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .channel = 6,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = 3,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    return ESP_OK;
}

esp_netif_t *get_example_netif_from_desc(const char *desc)
{
    esp_netif_t *netif = NULL;
    char *expected_desc;
    asprintf(&expected_desc, "%s: %s", TAG, desc);
    while ((netif = esp_netif_next(netif)) != NULL) {
        if (strcmp(esp_netif_get_desc(netif), expected_desc) == 0) {
            free(expected_desc);return netif;
        }
    }
    free(expected_desc);    return netif;
}

void wifi_stop(void)
{
    esp_netif_t *wifi_netif = get_example_netif_from_desc("sta");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect));
#endif
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);
    s_example_esp_netif = NULL;
}

esp_err_t wifi(void)
{
    // start();
    readWifi();
    esp_register_shutdown_handler(&stop);
    ESP_LOGI(TAG, "Waiting for IP(s)");
    for (int i = 0; i < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++i) {
        xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
    }
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip;
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next(netif);
        if (is_our_netif(TAG, netif)) {
            ESP_LOGI(TAG, "Connected to %s", esp_netif_get_desc(netif));
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
            ESP_LOGI(TAG, "- IPv4 address: " IPSTR, IP2STR(&ip.ip));
        }
    }
    return ESP_OK;
}



esp_err_t softap_init(void)
{
	// esp_err_t res = ESP_OK;
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *p_netif =  esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			// .ssid_len = strlen(WIFI_SSID),
			// .channel = 6,
			// .authmode = WIFI_AUTH_OPEN,
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,
			// .max_connection = 3,
			// .password = "ERD12345",
			.password = WIFI_PASS,
			// .password = "1a2b3c4d5e",
			// .pmf_cfg = {
            //     .capable = true,
            //     .required = false
            // },
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_connect());
	esp_netif_ip_info_t if_info;
	ESP_ERROR_CHECK(esp_netif_get_ip_info(p_netif, &if_info));
	ESP_LOGI("IP", "Address: " IPSTR, IP2STR(&if_info.ip));
	return ESP_OK;
}