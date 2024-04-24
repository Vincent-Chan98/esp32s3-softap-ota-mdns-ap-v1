#include "all_variables.h"
#include "rs485_custom.h"
#include "custom_mbcrc.h"
#include "csv_sd.h"

const uart_config_t uart_config = {
    .baud_rate = 115200,                      // correct
    .data_bits = UART_DATA_8_BITS,          // correct
    .parity = UART_PARITY_DISABLE,          // correct
    .stop_bits = UART_STOP_BITS_1,          // correct
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
};
#define EN1                         GPIO_NUM_6          // transmit
#define TXD1_PIN                    GPIO_NUM_4    // TX
#define RXD1_PIN                    GPIO_NUM_7    // RX
#define EN2                         GPIO_NUM_37         // receive
#define TXD2_PIN                    GPIO_NUM_35   // TX
#define RXD2_PIN                    GPIO_NUM_38   // RX
// #define EN1                         GPIO_NUM_37          // transmit
// #define TXD1_PIN                    GPIO_NUM_35    // TX
// #define RXD1_PIN                    GPIO_NUM_38    // RX
// #define EN2                         GPIO_NUM_6         // receive
// #define TXD2_PIN                    GPIO_NUM_4   // TX
// #define RXD2_PIN                    GPIO_NUM_7   // RX
static const int                    RX_BUF_SIZE = 1024;
 
uint8_t predawn_opn[15] = {0xFE, 
                    0x10, 
                    0x00, 0x00, 
                    0x00, 0x04, 
                    0x08, 
                    0x00, 0x7F, 0x00, 0x7F,
                    0x00, 0x00, 0x00, 0x00
                    };
uint8_t predawn_opn_mod[17];
uint8_t predown_cls[15] = {0xFE, 
                    0x10, 
                    0x00, 0x00, 
                    0x00, 0x04, 
                    0x08, 
                    0x00, 0x66, 0x00, 0x66,     // ---> 0xCC is 80% brighter
                    0x00, 0x00, 0x00, 0x00
                    };
uint8_t predown_cls_mod[17];
uint16_t predawn_opn_crc, predawn_cls_crc;
uint8_t pd_opn_HB, pd_opn_LB, pd_cls_HB, pd_cls_LB;

uint8_t morning_opn[15] = {0xFE, 
                    0x10, 
                    0x00, 0x00, 
                    0x00, 0x04, 
                    0x08, 
                    0x00, 0xFF, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00
                    };
uint8_t morning_opn_mod[17];
uint8_t morning_cls[15] = {0xFE, 
                    0x10, 
                    0x00, 0x00, 
                    0x00, 0x04, 
                    0x08, 
                    0x00, 0xCC, 0x00, 0x00,     // ---> 0xCC is 80% brighter
                    0x00, 0x00, 0x00, 0x00
                    };
uint8_t morning_cls_mod[17];
uint16_t morning_opn_crc, morning_cls_crc;
uint8_t m_opn_HB, m_opn_LB, m_cls_HB, m_cls_LB;

uint8_t afternoon_opn[15] = {0xFE, 
                    0x10, 
                    0x00, 0x00, 
                    0x00, 0x04, 
                    0x08, 
                    0x00, 0x00, 0x00, 0xFF,
                    0x00, 0x00, 0x00, 0x00
                    };
uint8_t afternoon_opn_mod[17];
uint8_t afternoon_cls[15] = {0xFE, 
                    0x10, 
                    0x00, 0x00, 
                    0x00, 0x04, 
                    0x08, 
                    0x00, 0x00, 0x00, 0xCC,     // ---> 0xCC is 80% brighter
                    0x00, 0x00, 0x00, 0x00
                    };
uint8_t afternoon_cls_mod[17];
uint16_t afternoon_opn_crc, afternoon_cls_crc;
uint8_t af_opn_HB, af_opn_LB, af_cls_HB, af_cls_LB;
// unsigned char test1[] = {0xFE, 0x10, 0x00, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB, 0xF5};
uint8_t temp[6]; 


int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1,data,len);
    // ESP_LOGI(logName,"Wrote %d bytes", txBytes);
    return txBytes;
}

int sendHex(const char* logName, const unsigned char* data, int len)
{
    // const int txBytes = uart_write_bytes(UART_NUM_1,data,len);
    const int txBytes = uart_write_bytes(UART_NUM_1,data,len);
    // ESP_LOGI(logName,"Wrote %d bytes", txBytes);
    // ESP_LOGI(logName,"Data %02X ", );
    // ESP_LOG_BUFFER_HEXDUMP(logName, data, len, ESP_LOG_INFO);
    return txBytes;
}

void h2str(unsigned int hexValue, char *output){sprintf(output,"%c",hexValue);}

void init485()
{
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 3, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);    
    uart_set_pin(UART_NUM_1,TXD2_PIN,RXD2_PIN,EN2,EN2);
    gpio_set_direction(EN2, GPIO_MODE_OUTPUT);
    gpio_set_level(EN2,1);
}

void color_setup(){
    // ------------------------ WAVE -------------------------
    uint8_t k=10;
    uint8_t j=k-1; 
	
    for (uint8_t m =0; m<(20/k); m++){
        for (uint8_t i = 0; i< k ; i++){
            bc[76+(k*(0+m)*3)+(i*3)+0] = value[3] - ((value[3]/k)*i);       
            bc[76+(k*(0+m)*3)+(i*3)+1] = value[4] - ((value[4]/k)*i);
            bc[76+(k*(0+m)*3)+(i*3)+2] = value[5] - ((value[5]/k)*i);

            bc[16+(k*m*3)+(i*3)+0] = value[3] - ((value[3]/k)*j);
            bc[16+(k*m*3)+(i*3)+1] = value[4] - ((value[4]/k)*j);
            bc[16+(k*m*3)+(i*3)+2] = value[5] - ((value[5]/k)*j);

            ac[16+(k*m*3)+(i*3)+0] = value[0] - ((value[0]/k)*i);
            ac[16+(k*m*3)+(i*3)+1] = value[1] - ((value[1]/k)*i);
            ac[16+(k*m*3)+(i*3)+2] = value[2] - ((value[2]/k)*i);

            ac[76+(k*(0+m)*3)+(i*3)+0] = value[0] - ((value[0]/k)*j);
            ac[76+(k*(0+m)*3)+(i*3)+1] = value[1] - ((value[1]/k)*j);
            ac[76+(k*(0+m)*3)+(i*3)+2] = value[2] - ((value[2]/k)*j);
            j--;
      	}
        j=k-1;
    }
    // ------------------------ WAVE -------------------------

    // ESP_LOG_BUFFER_HEXDUMP("COLORA", ac, 138, ESP_LOG_INFO);
    // ESP_LOG_BUFFER_HEXDUMP("COLORB", bc, 138, ESP_LOG_INFO);

    // ------------------------ FIRE -------------------------
    // for (uint8_t i = 0; i< 20 ; i++){
    //     bc[76+(i*3)+1] = 255 - abs(128 - rand() % value[4]);
    //     bc[76+(i*3)+0] = bc[76+(i*3)+1] / 2;     
    //     bc[76+(i*3)+2] = bc[76+(i*3)+1] / 4;

    //     bc[16+(i*3)+1] = 255 - abs(128 - rand() % value[4]);
    //     bc[16+(i*3)+0] = bc[16+(i*3)+1] / 2;
    //     bc[16+(i*3)+2] = bc[16+(i*3)+1] / 4;

    //     ac[16+(i*3)+1] = 255 - abs(128 - rand() % value[1]);
    //     ac[16+(i*3)+0] = ac[16+(i*3)+1] / 2;
    //     ac[16+(i*3)+2] = ac[16+(i*3)+1] / 4;

    //     ac[76+(i*3)+1] = 255 - abs(128 - rand() % value[1]);
    //     ac[76+(i*3)+0] = ac[76+(i*3)+1] / 2;
    //     ac[76+(i*3)+2] = ac[76+(i*3)+1] / 4;
    // }
    // ------------------------ FIRE -------------------------

    // ------------------------ BREATHING -----------------------
    // for (uint8_t i = 0; i < 40; i++)
    // {
    //     ac[16+(i*3)+0] = value[0];
    //     ac[16+(i*3)+1] = value[1];
    //     ac[16+(i*3)+2] = value[2];
    //     bc[16+(i*3)+0] = value[3];
    //     bc[16+(i*3)+1] = value[4];
    //     bc[16+(i*3)+2] = value[5];
    // }
    
    // ------------------------ BREATHING -----------------------
}

// void mid_open()
// {
//     a[256] = 0xAA;
//     a[257] = 0xBB;
//     uint16_t j = 0;
//         for (uint16_t k = 0; k < 240; k++)
//         {
//             a[16+k] = 0x00;
//         }
//         sendHex("TX_TASK", a, length1);
//         vTaskDelay(10/portTICK_PERIOD_MS);
//         for (uint16_t m = 30; m > 0; m--)
//         {            
//             a[12+(m*4)+0] = 0xFF;   //R
//             a[12+(m*4)+1] = 0x11 - (m*8);   //G
//             a[12+(m*4)+2] = 0x00 + (m*8);   //B
//             a[12+(m*4)+3] = 0x00;   //W
//             a[136+(j*4)+0] = 0x00;
//             a[136+(j*4)+1] = 0x11 - (m*8);
//             a[136+(j*4)+2] = 0x00 + (m*8);
//             a[136+(j*4)+3] = 0x00;
//             j++;
//             sendHex("TX_TASK", a, length1);    
//             vTaskDelay(50/portTICK_PERIOD_MS);  
//         }
//         j=0;  
// }



void mid_open_cob()
{
    ac[136] = 0xAA;
    ac[137] = 0xBB;
    
    
    // clear_all_cob();
//-------------------------------------------------------------------------------------
        // uint16_t j = 0;
        // for (uint16_t m = 20; m > 0; m--)
        // {            
        //     ac[13+(m*3)+0] = ac[16+(m*3)+0];   //R
        //     ac[13+(m*3)+1] = ac[16+(m*3)+0];   //G
        //     ac[13+(m*3)+2] = ac[16+(m*3)+0];   //B
              
        //     ac[76+(j*3)+0] = ac[73+(j*3)+0];
        //     ac[76+(j*3)+1] = ac[73+(j*3)+0];
        //     ac[76+(j*3)+2] = ac[73+(j*3)+0];
        //     j++;
        //     sendHex("TX_TASK", ac, length1);    
        //     vTaskDelay(time1[0]/portTICK_PERIOD_MS);  
        // }
        // j=0;  
//---------------------------------------------------------------------------------------
//*********************************   WAVE   OPEN   ***************************************
        uint16_t j = 18;
        temp[0] = ac[16];
        temp[1] = ac[16+1];
        temp[2] = ac[16+2];
        for (uint16_t m = 0; m <19; m++)
        {            
            ac[16+(m*3)+0] = ac[19+(m*3)+0];   //R
            ac[16+(m*3)+1] = ac[19+(m*3)+1];   //G
            ac[16+(m*3)+2] = ac[19+(m*3)+2];   //B
            ac[79+(j*3)+0] = ac[76+(j*3)+0];
            ac[79+(j*3)+1] = ac[76+(j*3)+1];
            ac[79+(j*3)+2] = ac[76+(j*3)+2];
            j--;
        }
        // j=18;
        ac[73] = temp[0];
        ac[73+1] = temp[1];
        ac[73+2] = temp[2];
        ac[76] = temp[0];
        ac[76+1] = temp[1];
        ac[76+2] = temp[2];
        
        sendHex("TX_TASK", ac, length1);    
        vTaskDelay(time1[0]/portTICK_PERIOD_MS); 

//***********************************   WAVE   OPEN   *********************************************

    // ------------------------ BREATHING -----------------------
    // uint8_t i = 0, j = 0;
    // for (; i < 40; i++)
    // {
    //     ac[16+(i*3)+0] = value[0]+j;
    //     ac[16+(i*3)+1] = value[1]+j;
    //     ac[16+(i*3)+2] = value[2]+j;
    //     j++;
    //     sendHex("TX_TASK", ac, length1);    
    //     vTaskDelay(time1[0]/portTICK_PERIOD_MS);
    // }
    
    // i = 0, j = 40;
    // for (; i < 40; i++)
    // {
    //     ac[16+(i*3)+0] = value[0]+j;
    //     ac[16+(i*3)+1] = value[1]+j;
    //     ac[16+(i*3)+2] = value[2]+j;
    //     j--;
    //     sendHex("TX_TASK", ac, length1);    
    //     vTaskDelay(time1[0]/portTICK_PERIOD_MS);
    // }
    // ------------------------ BREATHING -----------------------
}

void abab_cob()
{
    bc[136] = 0xAA;
    bc[137] = 0xBB;

    uint16_t j = 18;
    temp[3] = bc[73];
    temp[4] = bc[73+1];
    temp[5] = bc[73+2];

    for (uint16_t m = 0; m <19; m++)
    {            
        bc[19+(j*3)+0] = bc[16+(j*3)+0];   //R
        bc[19+(j*3)+1] = bc[16+(j*3)+1];   //G
        bc[19+(j*3)+2] = bc[16+(j*3)+2];   //B
        
        bc[76+(m*3)+0] = bc[79+(m*3)+0];
        bc[76+(m*3)+1] = bc[79+(m*3)+1];
        bc[76+(m*3)+2] = bc[79+(m*3)+2];
        j--;
    }

    bc[16] = temp[3];
    bc[16+1] = temp[4];
    bc[16+2] = temp[5];
    bc[133] = temp[3];
    bc[133+1] = temp[4];
    bc[133+2] = temp[5];
    
    sendHex("TX_TASK", bc, length2);    
    vTaskDelay(time1[1]/portTICK_PERIOD_MS);  
}

void clear_all_cob()
{
    cc[136] = 0xAA;
    cc[137] = 0xBB;
    for (uint16_t k = 0; k < 120; k++){cc[16+k] = 0x00;}
    sendHex("TX_TASK", cc, length3);
    vTaskDelay(30/portTICK_PERIOD_MS);  
}

uint16_t pd_opn_crc()
{
    uint16_t dam_len1 = sizeof(predawn_opn)/sizeof(predawn_opn[0]);
    predawn_opn_crc = usMBCRC16(predawn_opn,dam_len1);
    pd_opn_HB = (predawn_opn_crc >> 8) & 0xFF;
    pd_opn_LB = predawn_opn_crc & 0xFF;
    return predawn_opn_crc;
}

void pd_opn()
{
        pd_opn_crc();
        for (int i = 0; i < (sizeof(predawn_opn)/sizeof(predawn_opn[0])); i++)
        {
            predawn_opn_mod[i] = predawn_opn[i];
        }
        predawn_opn_mod[15] = pd_opn_HB;
        predawn_opn_mod[16] = pd_opn_LB;
        sendHex("DAM04L", predawn_opn_mod, (sizeof(predawn_opn_mod)/sizeof(predawn_opn_mod[0])));
        vTaskDelay(10/portTICK_PERIOD_MS);
}

uint16_t pd_cls_crc()
{
    uint16_t dam_len2 = sizeof(predown_cls)/sizeof(predown_cls[0]);
    predawn_cls_crc = usMBCRC16(predown_cls,dam_len2);
    pd_cls_HB = (predawn_cls_crc >> 8) & 0xFF;
    pd_cls_LB = predawn_cls_crc & 0xFF;
    return predawn_cls_crc;
}

void pd_cls()
{
        pd_cls_crc();
        for (int i = 0; i < (sizeof(predown_cls)/sizeof(predown_cls[0])); i++)
        {
            predown_cls_mod[i] = predown_cls[i];
        }
        predown_cls_mod[15] = pd_cls_HB;
        predown_cls_mod[16] = pd_cls_LB;
        sendHex("DAM04L", predown_cls_mod, (sizeof(predown_cls_mod)/sizeof(predown_cls_mod[0])));
        vTaskDelay(10/portTICK_PERIOD_MS);
}

uint16_t m_opn_crc()
{
    uint16_t dam_len1 = sizeof(morning_opn)/sizeof(morning_opn[0]);
    morning_opn_crc = usMBCRC16(morning_opn,dam_len1);
    m_opn_HB = (morning_opn_crc >> 8) & 0xFF;
    m_opn_LB = morning_opn_crc & 0xFF;
    return morning_opn_crc;
}

void m_opn()
{
        m_opn_crc();
        for (int i = 0; i < (sizeof(morning_opn)/sizeof(morning_opn[0])); i++)
        {
            morning_opn_mod[i] = morning_opn[i];
        }
        morning_opn_mod[15] = m_opn_HB;
        morning_opn_mod[16] = m_opn_LB;
        sendHex("DAM04L", morning_opn_mod, (sizeof(morning_opn_mod)/sizeof(morning_opn_mod[0])));
        vTaskDelay(10/portTICK_PERIOD_MS);
}

uint16_t m_cls_crc()
{
    uint16_t dam_len2 = sizeof(morning_cls)/sizeof(morning_cls[0]);
    morning_cls_crc = usMBCRC16(morning_cls,dam_len2);
    m_cls_HB = (morning_cls_crc >> 8) & 0xFF;
    m_cls_LB = morning_cls_crc & 0xFF;
    return morning_cls_crc;
}

void m_cls()
{
        m_cls_crc();
        for (int i = 0; i < (sizeof(morning_cls)/sizeof(morning_cls[0])); i++)
        {
            morning_cls_mod[i] = morning_cls[i];
        }
        morning_cls_mod[15] = m_cls_HB;
        morning_cls_mod[16] = m_cls_LB;
        sendHex("DAM04L", morning_cls_mod, (sizeof(morning_cls_mod)/sizeof(morning_cls_mod[0])));
        vTaskDelay(10/portTICK_PERIOD_MS);
}

uint16_t af_opn_crc()
{
    uint16_t dam_len1 = sizeof(afternoon_opn)/sizeof(afternoon_opn[0]);
    afternoon_opn_crc = usMBCRC16(afternoon_opn,dam_len1);
    af_opn_HB = (afternoon_opn_crc >> 8) & 0xFF;
    af_opn_LB = afternoon_opn_crc & 0xFF;
    return afternoon_opn_crc;
}

void af_opn()
{
        af_opn_crc();
        for (int i = 0; i < (sizeof(afternoon_opn)/sizeof(afternoon_opn[0])); i++)
        {
            afternoon_opn_mod[i] = afternoon_opn[i];
        }
        afternoon_opn_mod[15] = af_opn_HB;
        afternoon_opn_mod[16] = af_opn_LB;
        sendHex("DAM04L", afternoon_opn_mod, (sizeof(afternoon_opn_mod)/sizeof(afternoon_opn_mod[0])));
        vTaskDelay(10/portTICK_PERIOD_MS);
}

uint16_t af_cls_crc()
{
    uint16_t dam_len2 = sizeof(afternoon_cls)/sizeof(afternoon_cls[0]);
    afternoon_cls_crc = usMBCRC16(afternoon_cls,dam_len2);
    af_cls_HB = (afternoon_cls_crc >> 8) & 0xFF;
    af_cls_LB = afternoon_cls_crc & 0xFF;
    return afternoon_cls_crc;
}

void af_cls()
{
        af_cls_crc();
        for (int i = 0; i < (sizeof(afternoon_cls)/sizeof(afternoon_cls[0])); i++)
        {
            afternoon_cls_mod[i] = afternoon_cls[i];
        }
        afternoon_cls_mod[15] = af_cls_HB;
        afternoon_cls_mod[16] = af_cls_LB;
        sendHex("DAM04L", afternoon_cls_mod, (sizeof(afternoon_cls_mod)/sizeof(afternoon_cls_mod[0])));
        vTaskDelay(10/portTICK_PERIOD_MS);
}

void predawn()
{
        switch (data7)
        {
        case 0x0:            clear_all_cob();break;                                 // no opn and cls signal
        case 0x1:            pd_opn();while(data7!=0x2) {mid_open_cob();}break; // opn signal
        case 0x2:            pd_cls();abab_cob();break;                             // cls signal
        default:break;
        } 
}

void morning()
{
        switch (data7)
        {
        case 0x0:            clear_all_cob();break;
        case 0x1:            m_opn();while(data7!=0x2) {mid_open_cob();}break;
        case 0x2:            m_cls();abab_cob();break;        
        default:break;
        } 
}

void afternoon()
{
        switch (data7)
        {
        case 0x0:            clear_all_cob();break;
        case 0x1:            af_opn();while(data7!=0x2) {mid_open_cob();}break;
        case 0x2:            af_cls();abab_cob();break;        
        default:break;
        }     
}

