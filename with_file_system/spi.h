#ifndef __SPI_H__
#define __SPI_H__
#include <stdint.h>

/*
This file is part of avr-sd-interface (c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!

Please read the fine manual!

This file is for ATmega328P.

version 29.05.22
*/

void spi_init(void);
uint8_t spi_send_receive(const uint8_t byte);

#endif
