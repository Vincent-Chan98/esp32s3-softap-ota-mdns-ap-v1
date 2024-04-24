#include "all_variables.h"
#include "NTP_alls.h"
#include "rs485_custom.h"

#define NTPServer "pool.ntp.org"

enum eventTime {
    PREDAWN,        //0     --> 04:30 AM S
    MORNING,        //1     --> 08:25 AM
    NOON,           //2     --> 12:00 PM
    AFTERNOON,      //3     --> 16:00 PM
    NONE,           //4
};

#define T2 GPIO_NUM_17

void wifi_obtain_NTP()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTPServer);   //epoch
    sntp_init();
	ESP_LOGI("SNTP", "%u", sntp_enabled());
}

void NTP()
{
	gpio_set_direction(T2, GPIO_MODE_OUTPUT);
    enum eventTime spe_event;
    struct tm timeinfo0;
    ntp_time myTime;
    myTime.now0 = time(NULL);
    time_t stdrd, daylight;
    while(1){    
        setenv("TZ", "MYT-8", 1);
        tzset();      
        time(&myTime.now0);
        localtime_r(&myTime.now0, &timeinfo0);
        strftime(strftime_buf0, sizeof(strftime_buf0), "%A, %B %d %Y %H:%M:%S", &timeinfo0);
        // printf("%s\n",strftime_buf0);
        // printf("%d\n", timeinfo0.tm_wday); //70
        if (timeinfo0.tm_year/*1970*/ <= 80)
        {
            for (s_retry_num = 0; s_retry_num < retry_count; s_retry_num++)
            {
                printf("Waiting time to be set...\t(%d:%d)\n",s_retry_num,retry_count);
                vTaskDelay(1000/portTICK_PERIOD_MS);           
                time(&myTime.now0);localtime_r(&myTime.now0, &timeinfo0);     
            }         
            esp_restart();              
        }
		timeinfo0.tm_isdst = 0; // ---> with std time
        stdrd = mktime(&timeinfo0);
        timeinfo0.tm_isdst = 1; // ---> with day time
        daylight = mktime(&timeinfo0);
        diff = difftime(stdrd, daylight);  // ---> check time different
        if ((diff/60) >= 1){gpio_set_level(T2, 1);/*ESP_LOGI("NTP", "Time biased %f min(s)", diff/60);*/}
        if ((timeinfo0.tm_hour >= 4) && (timeinfo0.tm_hour < 8)){spe_event = PREDAWN;} 
        else if ((timeinfo0.tm_hour >= 8) && (timeinfo0.tm_hour < 12)){spe_event = MORNING;}
        else if ((timeinfo0.tm_hour >= 12) && (timeinfo0.tm_hour < 16)){spe_event = NOON;}
        else if ((timeinfo0.tm_hour >= 16)){spe_event = AFTERNOON;}
        else {spe_event = NONE;}
        switch (spe_event)
        {
            case PREDAWN:predawn();break;
            case MORNING:morning();break;
            case NOON:predawn();break;
            case AFTERNOON:afternoon();break;
            default:break;
        }
        vTaskDelay(1/portTICK_PERIOD_MS);
    }
}