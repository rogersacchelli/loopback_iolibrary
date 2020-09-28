/*
 * loopback_iolibrary.c
 *
 * Created: 27/09/2020 23:24:00
 * Author : Roger Sacchelli
 */ 

#define F_CPU 16000000UL


#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>

#include "usart.h"

#define DEBUG_ENABLE	1

// ********************************************************************************
// Global Variables
// ********************************************************************************
static FILE mystdout = FDEV_SETUP_STREAM(USART_putchar_printf, NULL, _FDEV_SETUP_WRITE);


int main(void)
{
    /* Replace with your application code */
	
	// setup our stdio stream
	stdout = &mystdout;
	
	// fire up the usart
	USART_init ( MYUBRR );
	

    while (1) 
    {
    }
}

