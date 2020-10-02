#include <avr/io.h>
#include "spi.h"

void SPI_MasterInit(void)
{
	/* Set MOSI, SCK output, SS output */
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_CS);
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

void SPI_MasterTransmit(uint8_t data)
{
	/* Start transmission */
	SPDR = data;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));
}

uint8_t SPI_MasterReceive(void)
{
	// transmit dummy byte
	SPDR = 0xFF;
	
	// Wait for reception complete
	while(!(SPSR & (1 << SPIF)));

	// return Data Register
	return SPDR;
}

void SPI_EnableTransmission(void){
	SPI_PORT &= ~(1<<PB0);
}

void SPI_DisableTransmission(void){
	SPI_PORT |= (1<<PB0);
}

