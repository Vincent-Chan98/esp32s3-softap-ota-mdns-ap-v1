#ifndef RS485_CUSTOM_H
#define RS485_CUSTOM_H

int sendData(const char* logName, const char* data);
int sendHex(const char* logName, const unsigned char* data, int len);
void h2str(unsigned int hexValue, char *output);
void init485();
void color_setup();
// void mid_open();
// void abab();
// void clear_all();
void mid_open_cob();
void abab_cob();
void clear_all_cob();
// void txb(void *arg); 
// void rx(void *arg);


uint16_t pd_opn_crc();
void pd_opn();
uint16_t pd_cls_crc();
void pd_cls();

uint16_t m_opn_crc();
void m_opn();
uint16_t m_cls_crc();
void m_cls();

uint16_t af_opn_crc();
void af_opn();
uint16_t af_cls_crc();
void af_cls();

void predawn();
void morning();
void afternoon();

#endif