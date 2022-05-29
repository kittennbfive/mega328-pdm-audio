#include <avr/io.h>

#include "spi.h"

/*
This file is part of avr-sd-interface (c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!

Please read the fine manual!

This file is for ATmega328P.

version 29.05.22
*/

void spi_init(void)
{
	DDRB|=(1<<PB2)|(1<<PB3)|(1<<PB5);
	SPCR=(1<<SPE)|(1<<MSTR);
	SPSR=(1<<SPI2X); //make it extra fast!
}

uint8_t spi_send_receive(const uint8_t v)
{
	SPDR=v;
	while(!(SPSR&(1<<SPIF)));
	return SPDR;
}
