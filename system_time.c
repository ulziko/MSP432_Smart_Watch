#include <time.h>
#include "system_time.h"


// this  function syncs  time with local time
void syncTime(){
       time_t current_time;
       current_time = time(NULL);
       struct tm *t = localtime(&current_time);
       time ( &current_time );
       ClockTime.hour = t->tm_hour;
       ClockTime.minute = t->tm_min;
       ClockTime.second = t->tm_sec;

       ClockDate.day   = t->tm_mday;
       ClockDate.month = t->tm_mon + 1;
       ClockDate.year  = t->tm_year + 1900;
}

// this function updates time every second
void updateTime(){
    ClockTime.second++;
    if (ClockTime.second >= 60) {
        ClockTime.second = 0;
        ClockTime.minute++;
        if (ClockTime.minute >= 60) {
            ClockTime.minute = 0;
            ClockTime.hour++;
            if (ClockTime.hour >= 24) {
                syncTime();
            }
        }
    }
}


