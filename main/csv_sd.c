// #include "sd_card.h"
#include "all_variables.h"
#include "csv_sd.h"
#include "rs485_custom.h"

// #define NTPServer "pool.ntp.org"

EventGroupHandle_t myEventGroup;

static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)12,(gpio_num_t)14,TWAI_MODE_NORMAL);
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_25KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

twai_message_t rx_message;
uint32_t tempidx, tempid;
esp_err_t err_tx;

void init_event_group(){myEventGroup = xEventGroupCreate();}


void XTEA_DEC(unsigned char *bufx)
{
    #define XTEA_ROUNDS         64
    #define XTEA_DELTA          0x9E3779B9
    unsigned long int k[4] = {(0x06010203), (0x08080108), (0x06010203), (0x08080108)}; //EITA_MCS_ID
    unsigned long v0, v1;
    long mask = 0xFF;
    unsigned long sum =XTEA_ROUNDS * XTEA_DELTA;
    unsigned long bufe[8];
    for (int t=0; t<8; t++){bufe[t] = bufx[t];}    
    v0 = (bufe[3] << 24 | bufe[2] << 16 | bufe[1] << 8 | bufe[0] << 0);
    v1 = (bufe[7] << 24 | bufe[6] << 16 | bufe[5] << 8 | bufe[4] << 0);
    for(int i=0; i < XTEA_ROUNDS; i++) {v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + k[sum>>11 & 3]);sum -= XTEA_DELTA;v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + k[sum & 3]);}    
    bufx[0] = v0>>0 & mask;    bufx[1] = v0>>8 & mask;    bufx[2] = v0>>16 & mask;    bufx[3] = v0>>24 & mask;
    bufx[4] = v1>>0 & mask;    bufx[5] = v1>>8 & mask;    bufx[6] = v1>>16 & mask;    bufx[7] = v1>>24 & mask;
}

void can_receive()
{
    twai_driver_install(&g_config, &t_config, &f_config);
    twai_start(); 
    twai_message_t rx_message;
    while(1){
        err_tx = twai_receive(&rx_message, pdMS_TO_TICKS(10));         //-->rer - report error on receive        
        if (err_tx == ESP_OK && rx_message.extd==1)//                                                                                                                         ABC         
        {
            can_flag = true;
            tempidx = rx_message.identifier;
            // XTEA_DEC(rx_message.data);
            if (tempidx==0x10082)
            {       
                switch (rx_message.data[7])
                {
                case 0x0:
                    data7 = rx_message.data[7];break;
                case 0x1:
                    data7 = rx_message.data[7];break;
                case 0x2:
                    data7 = rx_message.data[7];break;                
                default:
                    break;
                }                                                
            }
        } else{can_flag = false;}
        
        vTaskDelay(1/portTICK_PERIOD_MS);
    }
}

void cob()
{
    while (1)
    {
        switch (data7)
		{
		case 0x0:            clear_all_cob();break;
        case 0x1:            do{mid_open_cob();}while(data7!=0x2); break;
        case 0x2:            abab_cob();break;        
        default:break;
		}
        vTaskDelay(1/portTICK_PERIOD_MS);
    }
}