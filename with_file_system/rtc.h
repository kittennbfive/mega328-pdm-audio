#ifndef __RTC_H__
#define __RTC_H__
#include <stdint.h>

/*
dummy RTC-code for kittenFS32

always returns 01.01.1980 00:00

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY

version 29.05.22
*/

uint16_t rtc_get_encoded_date(void);
uint16_t rtc_get_encoded_time(void);

#endif
