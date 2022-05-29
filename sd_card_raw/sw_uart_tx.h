#ifndef __SW_UART_TX_H__
#define __SW_UART_TX_H__
#include <avr/io.h>
#include <stdio.h>

/*
Quick and dirty implementation of a basic software UART (TX only)

(c) 2022 by kittennbfive

AGPLv3+ and NO WARRANTY!
*/

//adjust these as needed
#define UART_DDR DDRB
#define UART_PORT PORTB
#define UART_OUT PB1

void sw_uart_tx_init(void);
int sw_uart_putchar(char c, FILE *stream);

#endif
