#include <avr/io.h>
#include <stdint.h>

#include "sd.h"

#include "spi.h"

/*
This file is part of avr-sd-interface (c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!

This code is suitable for use with kittenFS32.

Please read the fine manual!

version 29.05.22
*/

void sd_handle_io_error(const sd_error_t err); //you must provide this function somewhere

#define SD_CS_LOW PORT_SD_CS&=~(1<<SD_CS)
#define SD_CS_HIGH PORT_SD_CS|=(1<<SD_CS)

uint8_t sd_send_command(const uint8_t command, const uint8_t arg0, const uint8_t arg1, const uint8_t arg2, const uint8_t arg3, const uint8_t crc)
{
	spi_send_receive((0<<7)|(1<<6)|command);
	spi_send_receive(arg0);
	spi_send_receive(arg1);
	spi_send_receive(arg2);
	spi_send_receive(arg3);
	spi_send_receive(crc|1);
	uint8_t resp;
	do
	{
		resp=spi_send_receive(0xFF);
	} while(resp&(1<<7));
	
	return resp;
}

sd_init_result_t sd_init(void)
{
	/*
	CAUTION: According to the standard the SPI-speed should be <400kHz for init, but all the cards i tested accepted much more. Modify this if your card does not initialize!
	
	sequence inspired from https://electronics.stackexchange.com/questions/77417/what-is-the-correct-command-sequence-for-microsd-card-initialization-in-spi
	
	modified for compatibility with Sandisk:
		-pull CS high between CMD55 and ACMD41
		-add dummy-clocks between commands (with CS high)	
	*/
	
	SD_CS_HIGH;
	
	//dummy-clocks for startup
	uint8_t i;
	for(i=0; i<100; i++)
		spi_send_receive(0xFF);
	
	uint8_t resp;
	
	//CMD0 with CRC - returns R1
	uint8_t errorcount=0;
	do
	{
		for(i=0; i<10; i++)
			spi_send_receive(0xFF);
		
		SD_CS_LOW;
		resp=sd_send_command(0, 0x00, 0x00, 0x00, 0x00, 0x95);
		SD_CS_HIGH;
		
	} while(resp!=0x01 && ++errorcount<10);
	
	if(resp!=0x01)
		return SD_INIT_ERROR_CMD0;
	
	//dummy-clocks
	for(i=0; i<10; i++)
		spi_send_receive(0xFF);
	
	//CMD8 with CRC - returns R7 (5 bytes)
	SD_CS_LOW;
	resp=sd_send_command(8, 0x00, 0x00, 0x01, 0xAA, 0x87);
	(void)spi_send_receive(0xFF);
	(void)spi_send_receive(0xFF);
	(void)spi_send_receive(0xFF);
	(void)spi_send_receive(0xFF);
	SD_CS_HIGH;
	if(resp!=0x01)
		return SD_INIT_ERROR_CMD8;
	
	//dummy-clocks
	for(i=0; i<10; i++)
		spi_send_receive(0xFF);
	
	//ACMD41 = CMD55+"CMD41" no crc - returns R1 (x2)
	do
	{
		SD_CS_LOW;
		resp=sd_send_command(55, 0, 0, 0, 0, 0);
		SD_CS_HIGH;

		if(resp!=0x01)
			return SD_INIT_ERROR_CMD55;

		//dummy-clocks
		for(i=0; i<10; i++)
			spi_send_receive(0xFF);
		
		SD_CS_LOW;
		resp=sd_send_command(41, 0x40, 0, 0, 0, 0);
		SD_CS_HIGH;
		
		//dummy-clocks
		for(i=0; i<10; i++)
			spi_send_receive(0xFF);
		
	} while(resp==0x01);
	
	//CRC is off by default in SPI-mode
	
	//set blocklength to 512 with CMD16
	SD_CS_LOW;
	resp=sd_send_command(16, 0, 0, 2, 0, 0);
	SD_CS_HIGH;
	if(resp!=0x00)
		return SD_INIT_ERROR_CMD16;
	
	//dummy-clocks
	for(i=0; i<10; i++)
		spi_send_receive(0xFF);
	
	return SD_NO_ERROR;
}

void sd_read_sector(const uint32_t block, uint8_t * const ptr)
{
	//send dummy clock - IMPORTANT!
	spi_send_receive(0xFF);
	
	uint8_t resp;
	SD_CS_LOW;
	resp=sd_send_command(17, (block>>24)&0xFF, (block>>16)&0xFF, (block>>8)&0xFF, block&0xFF, 0x00);
	if(resp!=0x00)
	{
		SD_CS_HIGH;
		sd_handle_io_error(SD_READ_ERROR_CMD17);
	}
	
	do
	{
		resp=spi_send_receive(0xFF);
	} while(resp==0xFF);
	
	if(resp!=0xFE) //start token
	{
		SD_CS_HIGH;
		sd_handle_io_error(SD_READ_ERROR_START_TOKEN);
	}
	
	uint16_t i;
	for(i=0; i<512; i++)
		ptr[i]=spi_send_receive(0xFF);
	
	//crc must be received but will be ignored
	(void)spi_send_receive(0xFF);
	(void)spi_send_receive(0xFF);
	
	SD_CS_HIGH;
}

void sd_write_sector(const uint32_t block, uint8_t const * const ptr)
{
	//send dummy clock - IMPORTANT!
	spi_send_receive(0xFF);
	
	uint8_t resp;
	SD_CS_LOW;
	resp=sd_send_command(24, (block>>24)&0xFF, (block>>16)&0xFF, (block>>8)&0xFF, block&0xFF, 0x00);
	if(resp!=0x00)
	{
		SD_CS_HIGH;
		sd_handle_io_error(SD_WRITE_ERROR_CMD24);
	}
	
	spi_send_receive(0xFE); //start block token
	uint16_t i;
	for(i=0; i<512; i++)
	{
		spi_send_receive(ptr[i]);
	}
	
	do
	{
		resp=spi_send_receive(0xFF);
	} while(resp==0xFF);
	
	resp&=0x1F; //data response, NOT R1! mask undefined bits
	
	sd_error_t error=SD_NO_ERROR;
	if((resp&1)==0)
		error=SD_WRITE_INVALID_DATA_RESPONSE;
	else if((resp&0x0F)==0x0B)
		error=SD_WRITE_CRC_ERROR;
	else if((resp&0x0F)==0x0D)
		error=SD_WRITE_WRITE_ERROR;
	else if((resp&0x0F)!=0x05)
		error=SD_WRITE_UNKNOWN_ERROR;
	
	if(error!=SD_NO_ERROR)
	{
		SD_CS_HIGH;
		sd_handle_io_error(error);
	}
	
	do
	{
		resp=spi_send_receive(0xFF);
	} while(resp==0x00); //wait while card is busy
	
	SD_CS_HIGH;
}
