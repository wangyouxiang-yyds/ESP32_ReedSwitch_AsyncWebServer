#ifndef SLEEP_TIMER_H
#define SLEEP_TIMER_H

#include <time.h>

class SleepTimer {
    private:
        uint8_t tolerance; // 緩衝秒數
        const char* ntpServer = "pool.ntp.org";

        void goSleep(uint32_t sec){     // 設定喚醒時間並啟動深度睡眠
            // 喚醒時間 (微秒)
            esp_sleep_enable_timer_wakeup(sec * 1000000L);
            Serial.printf("喚醒秒數:%u\n", sec);
            Serial.printf("睡覺了~\n");
            delay(50);
            esp_deep_sleep_start(); // 進入睡眠
        }

    public:
        SleepTimer (uint8_t sec){
            tolerance = sec;
        }

        void init(uint16_t utcOffest=28800, uint8_t daylightOffest=0){
            configTime(utcOffest, daylightOffest, ntpServer);
        }

        int8_t start(uint8_t mn, uint8_t sec, void (*ptFunc)() ) {
            
        }



};



#endif