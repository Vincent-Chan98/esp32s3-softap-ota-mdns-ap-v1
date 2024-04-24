#include "heartbeat.h"



void heartbeat()
{
	gpio_config_t io_conf1 = {
		.pin_bit_mask = (1ULL << HB),
		.mode = GPIO_MODE_OUTPUT,
	};
	gpio_config(&io_conf1);
	gpio_set_direction(HB, GPIO_MODE_OUTPUT); 
	// gpio_set_direction(WF, GPIO_MODE_OUTPUT); 
	while (1)
	{
		if (can_flag == true){
			gpio_set_level(HB, 1);vTaskDelay(50/portTICK_RATE_MS);
			gpio_set_level(HB, 0);vTaskDelay(50/portTICK_RATE_MS);		
		}
		else{
			gpio_set_level(HB, 1);vTaskDelay(500/portTICK_RATE_MS);
			gpio_set_level(HB, 0);vTaskDelay(500/portTICK_RATE_MS);
		}
	}
}

void wifibeat()
{
	gpio_config_t io_conf2 = {
		.pin_bit_mask = (1ULL << WF),
		.mode = GPIO_MODE_OUTPUT,
	};
	gpio_config(&io_conf2);
	gpio_set_direction(WF, GPIO_MODE_OUTPUT); 
	while (1)
	{
		if (wf_flag == true){gpio_set_level(WF, 1);vTaskDelay(500/portTICK_RATE_MS);gpio_set_level(WF, 0);vTaskDelay(500/portTICK_RATE_MS);}
		else{gpio_set_level(WF, 0);}
	}
}