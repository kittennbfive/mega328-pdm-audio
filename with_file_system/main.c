#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "sw_uart_tx.h"

#include "spi.h"

#include "sd.h"

#include "FS32.h"

/*
This file is part of mega328-pdm-audio (c) 2022 by kittennbfive.

AGPLv3+ and NO WARRANTY!

Please read the fine manual!

version 1 - 29.05.22
*/

#define VALUE_UBBR 11 //ADJUST THIS! (see documentation)


#define SZ_BUFFER 512 //do not change unless you want to hack!

void sd_handle_io_error(const sd_error_t err)
{
	(void)err;
	printf_P(PSTR("SD_ERROR %u\r\n"), (uint8_t)err);
	while(1);
}

//you can remove this stuff if you dont need it
#define DDR_DEBUG DDRC
#define DEBUG PORTC
#define DBG0 PC0
#define DBG1 PC1
#define DBG2 PC2

static volatile uint8_t buffer1[SZ_BUFFER];
static volatile uint8_t buffer2[SZ_BUFFER];
static volatile uint8_t * buffer_ptr_out=buffer1;
static volatile uint8_t * buffer_ptr_sd=buffer2;

static volatile uint16_t index_b1=0;
static volatile uint16_t index_b2=0;
static volatile uint16_t * index_ptr_out=&index_b1;
static volatile uint16_t * index_ptr_sd=&index_b2;

static volatile bool fill_buffer=false;

ISR(USART_UDRE_vect)
{
	UDR0=buffer_ptr_out[(*index_ptr_out)++];
	if((*index_ptr_out)==SZ_BUFFER)
	{
		volatile uint8_t * tmp_ptr=buffer_ptr_out;
		buffer_ptr_out=buffer_ptr_sd;
		buffer_ptr_sd=tmp_ptr;

		volatile uint16_t * tmp_index_ptr=index_ptr_out;
		index_ptr_out=index_ptr_sd;
		index_ptr_sd=tmp_index_ptr;

		fill_buffer=true;
	}
}

int main(void)
{
	DDR_DEBUG|=(1<<DBG0)|(1<<DBG1)|(1<<DBG2);

	//USART in SPI-mode
	DDRD|=(1<<PD4)|(1<<PD1);
	UBRR0=0;
	UCSR0B=(1<<TXEN0)|(1<<UDRIE0);
	UCSR0C=(1<<UMSEL01)|(1<<UMSEL00);
	UBRR0=VALUE_UBBR;

	//software UART
	sw_uart_tx_init();
	FILE uart_output = FDEV_SETUP_STREAM(sw_uart_putchar, NULL, _FDEV_SETUP_WRITE);
	stdout = &uart_output;


	printf_P(PSTR("\r\nThis is mega328-pdm-audio by kittennbfive\r\n\r\n"));

	printf_P(PSTR("initializing SD and filesystem...\r\n"));

	spi_init();
	sd_init_result_t res_init=sd_init();

	if(res_init)
	{
		printf_P(PSTR("sd_init failed: %u\r\n"), (uint8_t)res_init);
		while(1);
	}

	FS32_status_t status;
	status=f_init();
	if(status)
	{
		printf_P(PSTR("f_init failed: %u\r\n"), (uint8_t)status);
		while(1);
	}

	uint8_t dummy;
	status=f_open(&dummy, "PDM.BIN", 'r');
	if(status)
	{
		printf_P(PSTR("f_open failed: %u\r\n"), (uint8_t)status);
		while(1);
	}

	status=f_read(dummy, (uint8_t*)buffer1, SZ_BUFFER, 1);
	if(status)
	{
		printf_P(PSTR("initial f_read buffer1 failed: %u\r\n"), (uint8_t)status);
		while(1);
	}

	status=f_read(dummy, (uint8_t*)buffer2, SZ_BUFFER, 1);
	if(status)
	{
		printf_P(PSTR("initial f_read buffer2 failed: %u\r\n"), (uint8_t)status);
		while(1);
	}

	printf_P(PSTR("starting playback\r\n"));

	sei();
	
	while(1)
	{
		if(fill_buffer)
		{
			fill_buffer=false;
			(*index_ptr_sd)=0;
			DEBUG|=(1<<DBG0);
			status=f_read(dummy, (uint8_t*)buffer_ptr_sd, SZ_BUFFER, 1);
			DEBUG&=~(1<<DBG0);
			if(status)
			{
				cli();
				break;
			}
		}
	}

	f_close(dummy);

	printf_P(PSTR("finished, going into endless loop\r\n"));

	while(1);

	return 0;
}
