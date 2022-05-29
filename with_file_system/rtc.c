#include <stdint.h>

#include "rtc.h"

/*
dummy RTC-code for kittenFS32

always returns 01.01.1980 00:00

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY

version 29.05.22
*/

uint16_t rtc_get_encoded_date(void)
{
	uint8_t year, month, day;
	
	year=0; //offset from 1980
	month=1;
	day=1;
	
	return ((uint16_t)(year&0x7F)<<9)|((month&0x0F)<<5)|((day&0x1F)<<0);
}

uint16_t rtc_get_encoded_time(void)
{
	uint8_t h,m,s;
	
	h=0;
	m=0;
	s=0;
	
	return ((uint16_t)(h&0x1F)<<11)|((m&0x3F)<<5)|(((s/2)&0x1F)<<0);
}
