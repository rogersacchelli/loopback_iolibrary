#include <stdio.h>
#include <avr/io.h>

void USART_init( uint16_t ubrr) {
	
	// For initialization, 2560 have its default parameters for 8-bit / 1-stop bit
	
	// Set baud rate
	UBRR0H = (uint8_t)(ubrr>>8);
	UBRR0L = (uint8_t)ubrr;
	// Enable receiver and transmitter
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
}

void USART_Transmit( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	;
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

unsigned char USART_Receive( void )
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) );
	/* Get and return received data from buffer */
	return UDR0;
}

unsigned char USART_kbhit(void) {
	//return nonzero if char waiting polled version
	unsigned char b;
	b=0;
	if(UCSR0A & (1<<RXC0)) b=1;
	return b;
}

void USART_pstr(char *s) {
	// loop through entire string
	while (*s) {
		USART_Transmit(*s);
		s++;
	}
}

// this function is called by printf as a stream handler
int USART_putchar_printf(char var, FILE *stream) {
	// translate \n to \r for br@y++ terminal
	if (var == '\n') USART_Transmit('\r');
	USART_Transmit(var);
	return 0;
}