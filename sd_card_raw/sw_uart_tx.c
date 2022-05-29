#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <util/delay.h>

#include "sw_uart_tx.h"

/*
Quick and dirty implementation of a basic software UART (TX only)

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

#define BITDELAY_US 26 //this needs to be an integer! 26us is the lowest value for a standard baudrate (38400)

#define UART_LOW UART_PORT&=~(1<<UART_OUT)
#define UART_HIGH UART_PORT|=(1<<UART_OUT)

#define UART_DELAY _delay_us(BITDELAY_US)

void sw_uart_tx_init(void)
{
	UART_DDR|=(1<<UART_OUT);
	UART_HIGH; //idle high
	UART_DELAY;
}

int sw_uart_putchar(char c, FILE *stream)
{
	(void)stream;
	
	//startbit
	UART_LOW;
	UART_DELAY;
	
	//data
	uint8_t i;
	for(i=0; i<8; i++)
	{
		if(c&(1<<0))
			UART_HIGH;
		else
			UART_LOW;
		UART_DELAY;
		c>>=1;
	}
	
	//stopbit
	UART_HIGH;
	UART_DELAY;
	
	return 0;
}
