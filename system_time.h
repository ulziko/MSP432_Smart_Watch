/*
 * time_display.h
 *
 *  Created on: 15 dec 2025
 *      Author: Ulzii
 */

#ifndef SYSTEM_TIME_H_
#define SYSTEM_TIME_H_

    typedef struct {
        int hour;
        int minute;
        int second;
        char time_string[20];
    } TimeStruct;

    typedef struct{
        int day;
        int month;
        int year;
        char date_string[20];
    } DateStruct;

    // Variables used accross the system
    extern TimeStruct ClockTime;
    extern DateStruct ClockDate;

    void syncTime(void);
    // this function is called when our  TA0 interrupt happens
    void updateTime(void);
#endif /* SYSTEM_TIME_H_ */
