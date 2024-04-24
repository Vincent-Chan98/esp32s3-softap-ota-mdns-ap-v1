/* MDNS-SD Query and advertise Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "inttypes.h"

#include "all_variables.h"
#include "csv_sd.h"
#include "NTP_alls.h"
#include "rs485_custom.h"
#include "sd_card.h"
#include "heartbeat.h"

#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include <esp_http_server.h>

#include "esp_netif_ip_addr.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_netif_sta_list.h"

#include "mdns.h"
#include "driver/gpio.h"
#include "netdb.h"
#include <esp_ota_ops.h>
#include <sys/param.h>
#include "hal/twai_types.h"
#include "esp_spi_flash.h"

#include "lwip/apps/sntp.h" 
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_efuse.h"
#include "spinlock.h"

#define EXAMPLE_MDNS_HOSTNAME "eitahq-lift3"
#define EXAMPLE_MDNS_INSTANCE "eitahq-lift3-mdns"
#define ALIVE GPIO_NUM_9
#define FAN1 GPIO_NUM_10
#define BELL1 GPIO_NUM_11
#define MOUNT_POINT "/sdcard"
/*
 * Serve OTA update portal (index.html)
 */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t eita_jpg_start[] asm("_binary_eita_jpg_start");
extern const uint8_t eita_jpg_end[] asm("_binary_eita_jpg_end");

static const char * TAG = "mdns-test";
bool opn = false, cls = false;
bool rbt = false;
uint8_t rbt_val = 0;
uint8_t ledstate = 0, fan = 0, bell = 0;
const char *dataupdate[3] = {"OPEN Updated", "CLOSE Updated", " "};
esp_err_t wf_err;

static void initialise_mdns(void)
{
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(EXAMPLE_MDNS_HOSTNAME) );
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", EXAMPLE_MDNS_HOSTNAME);
    //set default mDNS instance name - Description
    ESP_ERROR_CHECK( mdns_instance_name_set(EXAMPLE_MDNS_INSTANCE) );

    //structure with TXT records
    mdns_txt_item_t serviceTxtData[3] = {
        {"board", "esp32"},
        {"u", "user"},
        {"p", "password"}
    };

    //initialize service
    ESP_ERROR_CHECK( mdns_service_add("eitahq-lift3-WebServer", "_http", "_tcp", 80, serviceTxtData, 3) );
#if CONFIG_MDNS_MULTIPLE_INSTANCE
    ESP_ERROR_CHECK( mdns_service_add("eitahq-lift3-WebServer1", "_http", "_tcp", 80, NULL, 0) );
#endif

    struct hostent *hp;
    hp = gethostbyname(EXAMPLE_MDNS_HOSTNAME);
    if (hp == NULL)    {ESP_LOGI(TAG,"Cannot resolve host");}
	ESP_LOGI(TAG,"Host has been resolved");

    //add another TXT item
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "path", "/foobar") );
    //change TXT item value
    ESP_ERROR_CHECK( mdns_service_txt_item_set_with_explicit_value_len("_http", "_tcp", "u", "admin", strlen("admin")) );
}

esp_err_t index_get_handler(httpd_req_t *req)
{
	httpd_resp_send(req, (const char *) index_html_start, index_html_end - index_html_start);
	return ESP_OK;
}

esp_err_t jpg_get_handler(httpd_req_t *req)
{
	httpd_resp_send(req, (const char *) eita_jpg_start, eita_jpg_end - eita_jpg_start);
	return ESP_OK;
}
/*
 * Handle OTA file upload
 */
esp_err_t update_post_handler(httpd_req_t *req)
{
	char buf[1000];
	esp_ota_handle_t ota_handle;
	int remaining = req->content_len;
	const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
	ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));
	while (remaining > 0) {
		int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));

		// Timeout Error: Just retry
		if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {continue;
		// Serious Error: Abort OTA
		} else if (recv_len <= 0) {httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Protocol Error");return ESP_FAIL;
		}
		// Successful Upload: Flash firmware chunk
		if (esp_ota_write(ota_handle, (const void *)buf, recv_len) != ESP_OK) {
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Flash Error");return ESP_FAIL;
		}
		remaining -= recv_len;
	}
	// Validate and switch to new OTA image and reboot
	if (esp_ota_end(ota_handle) != ESP_OK || esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation / Activation Error");
			return ESP_FAIL;
	}
	httpd_resp_sendstr(req, "Firmware update complete, rebooting now!\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);
	esp_restart();
	return ESP_OK;
}

esp_err_t data_get_open_handler(httpd_req_t *req)
{
	all_handle c_opn;
	int j=0;
	const char *opndata = req->uri;
	printf("OPEN: %s\n", opndata);
	char *opnval = strtok(strdup(opndata),"=");
	opnval = strtok(NULL,"=");
	printf("%s\n", opnval);
	
	char *opntemp = strtok(opnval, "&");
	char *opntemp1[4];
	while (opntemp!=NULL)	{opntemp1[j++] = opntemp;opntemp = strtok(NULL, "&");}
	for (j = 0; j < (sizeof(opntemp1)/sizeof(opntemp1[0]))-1; ++j)
	{
		nvs_open("value_s"+j, NVS_READWRITE, &c_opn.c_value_opn);
		value[j] = (unsigned char)strtol(opntemp1[j],NULL,16);
		nvs_set_i32(c_opn.c_value_opn, "abab"+j, (int32_t)value[j]);
		nvs_commit(c_opn.c_value_opn);
		nvs_close(c_opn.c_value_opn);
		printf("Open: %02X\n", value[j]);
	}	
	nvs_open("time_s"+0, NVS_READWRITE,&c_opn.time_mid_opn);
	time1[0] = atoi(opntemp1[3]);           
	nvs_set_i32(c_opn.time_mid_opn, "ani"+0, (int32_t)time1[0]);
	nvs_commit(c_opn.time_mid_opn);   
	nvs_close(c_opn.time_mid_opn);

	// if (checkSD() == ESP_OK)
	// {
	// 	printf("Get into SD OPN region\n");
	// 	fseek(configFile, 0, SEEK_SET);
	// 	fprintf(configFile,"0x%02X\n",value[0]); 
	// 	fprintf(configFile,"0x%02X\n",value[1]);
	// 	fprintf(configFile,"0x%02X\n",value[2]);
	// 	fprintf(configFile,"%d",time1[0]);
	// 	fclose(configFile);fclose(pFile);
	// 	printf("Done writing to SD value and time\n");		
	// 	color_setup();
	// 	httpd_resp_sendstr(req, "OPEN color update complete\n");
	// 	vTaskDelay(500 / portTICK_PERIOD_MS);
	// 	return ESP_OK;
	// }

	color_setup();
	opn = true;cls = false;
	// httpd_resp_sendstr(req, "OPEN updated\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);
	// esp_restart();
	return ESP_OK;
}

esp_err_t data_get_close_handler(httpd_req_t *req)
{
	all_handle c_cls;
	int k=0;
	const char *clsdata = req->uri;
	printf("CLOSE: %s\n", clsdata);
	char *clsval = strtok(strdup(clsdata),"=");
	clsval = strtok(NULL,"=");
	printf("%s\n", clsval);
	
	char *clstemp = strtok(clsval, "&");
	char *clstemp1[4];
	while (clstemp!=NULL)
	{
		clstemp1[k++] = clstemp;
		clstemp = strtok(NULL, "&");
	}
	for (k = 0; k < (sizeof(clstemp1)/sizeof(clstemp1[0]))-1; ++k)
	{
		nvs_open("value_s"+k+3, NVS_READWRITE, &c_cls.c_value_cls);
		value[k+3] = (unsigned char)strtol(clstemp1[k],NULL,16);
		nvs_set_i32(c_cls.c_value_cls, "abab"+k+3, (int32_t)value[k+3]);
		nvs_commit(c_cls.c_value_cls);
		nvs_close(c_cls.c_value_cls);
		printf("Close %02X\n", value[k+3]);
	}	
	nvs_open("time_s"+1, NVS_READWRITE,&c_cls.time_mid_cls);
	time1[1] = atoi(clstemp1[3]);           
	nvs_set_i32(c_cls.time_mid_cls, "ani"+1, (int32_t)time1[1]);
	nvs_commit(c_cls.time_mid_cls);   
	nvs_close(c_cls.time_mid_cls);

	// if (checkSD() == ESP_OK)
	// {
	// 	printf("Get into SD CLS region\n");
	// 	fseek(configFile, 0, SEEK_SET);
	// 	while (fscanf(configFile, "%s\n", buff1) != EOF)
	// 	{
	// 		m++;
	// 		if (m == 5)
	// 		{
	// 			fseek(configFile, 0, SEEK_CUR);
	// 			fprintf(configFile,"0x%02X\n",value[3]); 
	// 			fprintf(configFile,"0x%02X\n",value[4]);
	// 			fprintf(configFile,"0x%02X\n",value[5]);
	// 			fprintf(configFile,"%d",time1[1]);
	// 			fclose(configFile);fclose(pFile);
	// 			break;
	// 		}			
	// 	}
	// 	printf("Done writing to SD value and time\n");		
	// 	color_setup();
	// 	httpd_resp_sendstr(req, "OPEN color update complete\n");
	// 	vTaskDelay(500 / portTICK_PERIOD_MS);
	// 	return ESP_OK;
	// }

	color_setup();
	cls = true;opn = false;
	// httpd_resp_sendstr(req, "CLOSE color update complete\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);
	// esp_restart();
	return ESP_OK;
}

esp_err_t reboot_handler(httpd_req_t *req)
{
	all_handle rbthld;
	rbt = true;
	nvs_open("rbt", NVS_READWRITE,&rbthld.rbt_esp);    
	nvs_set_u8(rbthld.rbt_esp, "rbt", (uint8_t)rbt);
	nvs_commit(rbthld.rbt_esp);   
	nvs_close(rbthld.rbt_esp);
	esp_restart();
	return ESP_OK;
}

esp_err_t data_show_handler(httpd_req_t *req)
{
	char dataShow[160];
	all_handle showhdl;
	for (int n = 0; n < (sizeof(value)/sizeof(value[0])); n++)
	{
		nvs_open("value_s"+n, NVS_READONLY, &showhdl.value_mid);
		nvs_get_i32(showhdl.value_mid, "abab"+n, (int32_t*)&value[n]);
		nvs_close(showhdl.value_mid);
	}
	for (int p = 0; p < (sizeof(time1)/sizeof(time1[0])); p++)
	{
		nvs_open("time_s"+p, NVS_READONLY, &showhdl.time_mid);
		nvs_get_i32(showhdl.time_mid, "ani"+p, (int32_t*)&time1[p]);
		nvs_close(showhdl.time_mid);
	}
	snprintf(dataShow, sizeof(dataShow), "<center><p><strong>Open --> G:0x%02X   R:0x%02X   B:0x%02X   T:%d ms<br>Close --> G:0x%02X   R:0x%02X   B:0x%02X   T:%d ms</strong></p></center>", value[0], value[1], value[2], time1[0], value[3], value[4], value[5], time1[1]);
	httpd_resp_send(req, dataShow, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

esp_err_t led_handler(httpd_req_t *req)
{
	if (ledstate == 0)
	{
		gpio_set_level(ALIVE, 0); ledstate = 1; return ESP_OK;
	} else {
		gpio_set_level(ALIVE, 1); ledstate = 0; return ESP_OK;
	}
}

esp_err_t open_update_handler(httpd_req_t *req)
{
	if (opn == true)
	{
		httpd_resp_send(req, dataupdate[0], HTTPD_RESP_USE_STRLEN);		
		return ESP_OK;
	}
	else{
		httpd_resp_send(req, dataupdate[2], HTTPD_RESP_USE_STRLEN);
		return ESP_OK;
	}
}
esp_err_t close_update_handler(httpd_req_t *req)
{
	if (cls == true)
	{
		httpd_resp_send(req, dataupdate[1], HTTPD_RESP_USE_STRLEN);		
		return ESP_OK;
	}
	else{
		httpd_resp_send(req, dataupdate[2], HTTPD_RESP_USE_STRLEN);
		return ESP_OK;
	}
}

esp_err_t espreboot_handler(httpd_req_t *req)
{
	all_handle rbthld;
	nvs_open("rbt", NVS_READONLY,&rbthld.rbt_esp);    
	nvs_get_u8(rbthld.rbt_esp, "rbt", &rbt_val); 
	nvs_close(rbthld.rbt_esp);
	if (rbt_val == false)
	{
		httpd_resp_send(req, " ", HTTPD_RESP_USE_STRLEN);
		// return ESP_OK;	
	} else if (rbt_val == true) {		
		httpd_resp_send(req, "Rebooted", HTTPD_RESP_USE_STRLEN);
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		rbt = false;
		nvs_open("rbt", NVS_READWRITE,&rbthld.rbt_esp);     
		nvs_set_u8(rbthld.rbt_esp, "rbt", (uint8_t)rbt);
		nvs_commit(rbthld.rbt_esp);   
		nvs_close(rbthld.rbt_esp);
	} 
	return ESP_OK;
}

esp_err_t fan_handler(httpd_req_t *req)
{
	if (fan == 0)
	{
		gpio_set_level(FAN1, 0); fan = 1; return ESP_OK;
	} else {
		gpio_set_level(FAN1, 1); fan = 0; return ESP_OK;
	}
}

esp_err_t alarm_handler(httpd_req_t *req)
{
	if (bell == 0)
	{
		gpio_set_level(BELL1, 0); bell = 1; return ESP_OK;
	} else {
		gpio_set_level(BELL1, 1); bell = 0; return ESP_OK;
	}
}

/*
 * HTTP Server
 */
httpd_uri_t index_get = {
	.uri	  = "/",
	.method   = HTTP_GET,
	.handler  = index_get_handler,
	.user_ctx = NULL
};
httpd_uri_t jpg_get = {
	.uri	  = "/eita.jpg",
	.method   = HTTP_GET,
	.handler  = jpg_get_handler,
	.user_ctx = NULL
};
httpd_uri_t update_post = {
	.uri	  = "/update",
	.method   = HTTP_POST,
	.handler  = update_post_handler,
	.user_ctx = NULL
};
httpd_uri_t data_get_open = {
	.uri	  = "/data/open/",
	.method   = HTTP_GET,
	.handler  = data_get_open_handler,
	.user_ctx = NULL
};
httpd_uri_t data_get_close = {
	.uri	  = "/data/close/",
	.method   = HTTP_GET,
	.handler  = data_get_close_handler,
	.user_ctx = NULL
};
httpd_uri_t reboot = {
	.uri	  = "/reboot",
	.method   = HTTP_GET,
	.handler  = reboot_handler,
	.user_ctx = NULL
};
httpd_uri_t data_show = {
	.uri	  = "/data/show",
	.method   = HTTP_GET,
	.handler  = data_show_handler,
	.user_ctx = NULL
};
httpd_uri_t ledControl = {
	.uri	  = "/controlled",
	.method   = HTTP_GET,
	.handler  = led_handler,
	.user_ctx = NULL
};
httpd_uri_t opn_update = {
	.uri	  = "/open/update",
	.method   = HTTP_GET,
	.handler  = open_update_handler,
	.user_ctx = NULL
};
httpd_uri_t cls_update = {
	.uri	  = "/close/update",
	.method   = HTTP_GET,
	.handler  = close_update_handler,
	.user_ctx = NULL
};
httpd_uri_t esp_reboot = {
	.uri	  = "/esp/reboot",
	.method   = HTTP_GET,
	.handler  = espreboot_handler,
	.user_ctx = NULL
};
httpd_uri_t fanControl = {
	.uri	  = "/fan",
	.method   = HTTP_GET,
	.handler  = fan_handler,
	.user_ctx = NULL
};
httpd_uri_t alarmControl = {
	.uri	  = "/alarm",
	.method   = HTTP_GET,
	.handler  = alarm_handler,
	.user_ctx = NULL
};

static esp_err_t http_server_init(void)
{
	static httpd_handle_t http_server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers = 14;
	if (httpd_start(&http_server, &config) == ESP_OK) {
		httpd_register_uri_handler(http_server, &index_get);
		httpd_register_uri_handler(http_server, &jpg_get);
		httpd_register_uri_handler(http_server, &update_post);
		httpd_register_uri_handler(http_server, &data_get_open);
		httpd_register_uri_handler(http_server, &data_get_close);
		httpd_register_uri_handler(http_server, &reboot);
		httpd_register_uri_handler(http_server, &data_show);
		// httpd_register_uri_handler(http_server, &ledControl);
		httpd_register_uri_handler(http_server, &opn_update);
		httpd_register_uri_handler(http_server, &cls_update);
		httpd_register_uri_handler(http_server, &esp_reboot);
		// httpd_register_uri_handler(http_server, &fanControl);
		// httpd_register_uri_handler(http_server, &alarmControl);
	}
	return http_server == NULL ? ESP_FAIL : ESP_OK;
}

// void led_blink(){
// 	while (1)
// 	{
// 		// for (int k = 0; k < 3; k++)		{printf("value_AF %d - %02X\n", k, value[k]);}
// 		// for (int k = 6; k < 9; k++)		{printf("value_AF %d - %02X\n", k, value[k]);}
// 		gpio_set_level(ALIVE, 0);		vTaskDelay(1000/portTICK_PERIOD_MS);
// 		gpio_set_level(ALIVE, 1);		vTaskDelay(1000/portTICK_PERIOD_MS);
// 	}
// }

void app_main(void)
{
    gpio_set_direction(ALIVE, GPIO_MODE_OUTPUT);
	gpio_set_direction(FAN1, GPIO_MODE_OUTPUT);
	gpio_set_direction(BELL1, GPIO_MODE_OUTPUT);
	gpio_set_direction(SD, GPIO_MODE_OUTPUT);
	gpio_set_direction(BK, GPIO_MODE_OUTPUT);

	gpio_set_level(HB, 1);gpio_set_level(SD, 1);gpio_set_level(BK, 1);gpio_set_level(WF, 1);vTaskDelay(1500/portTICK_RATE_MS);
	gpio_set_level(HB, 0);gpio_set_level(SD, 0);gpio_set_level(BK, 0);gpio_set_level(WF, 0);

    esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init485();
	sdcard();

    initialise_mdns();
    // ESP_ERROR_CHECK(wifi());
	wf_err = readWifi();
	if (wf_err == ESP_OK)    {wf_flag = true;}
    else {wf_flag = false;}

    ESP_ERROR_CHECK(http_server_init());
    /* Mark current app as valid */
	const esp_partition_t *partition = esp_ota_get_running_partition();
	printf("Currently running partition: %s\r\n", partition->label);
	esp_ota_img_states_t ota_state;
	if (esp_ota_get_state_partition(partition, &ota_state) == ESP_OK) {
		if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
			esp_ota_mark_app_valid_cancel_rollback();
		}
	}
    // wifi_obtain_NTP();
	esp_err_t fr = fileRead();
	color_setup();

	if (fr==ESP_FAIL || fr==ESP_OK)
	{
		xTaskCreatePinnedToCore(cob, "cob", 10000, NULL, 1, NULL, 0); 
		xTaskCreatePinnedToCore(heartbeat, "hb", 2048, NULL, 1, NULL, 1);
		xTaskCreatePinnedToCore(wifibeat, "wfb", 2048, NULL, 1, NULL, 1);
		xTaskCreatePinnedToCore(can_receive, "can_r", 4096, NULL, 1, NULL, 1); 
	}
}
