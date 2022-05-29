#ifndef __SD_H__
#define __SD_H__
#include <avr/io.h>
#include <stdint.h>

/*
This file is part of avr-sd-interface (c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!

This code is suitable for use with kittenFS32.

Please read the fine manual!

version 29.05.22
*/

//adjust this
#define DDR_SD_CS DDRB
#define PORT_SD_CS PORTB
#define SD_CS PB2

typedef enum
{
	SD_INIT_NO_ERROR=0,
	SD_INIT_ERROR_CMD0,
	SD_INIT_ERROR_CMD8,
	SD_INIT_ERROR_CMD16,
	SD_INIT_ERROR_CMD55
} sd_init_result_t;

typedef enum
{
	SD_NO_ERROR=0,
	
	SD_READ_ERROR_CMD17,
	SD_READ_ERROR_START_TOKEN,
	SD_WRITE_ERROR_CMD24,
	SD_WRITE_INVALID_DATA_RESPONSE,
	SD_WRITE_CRC_ERROR,
	SD_WRITE_WRITE_ERROR,
	SD_WRITE_UNKNOWN_ERROR
} sd_error_t;

sd_init_result_t sd_init(void);
void sd_read_sector(const uint32_t sector, uint8_t * const data);
void sd_write_sector(const uint32_t sector, uint8_t const * const data);

#endif
