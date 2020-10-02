#include <avr/io.h>

#define DDR_SPI		DDRB
#define SPI_PORT	PORTB

#define DD_CS      PINB0
#define DD_SCK     DDB1
#define DD_MOSI    DDB2

#define CS      PINB0
#define SCK     PINB1
#define MOSI    PINB2
#define MISO    PINB3

void SPI_MasterInit(void);

void SPI_MasterTransmit(uint8_t data);

uint8_t SPI_MasterReceive(void);

void SPI_EnableTransmission(void);

void SPI_DisableTransmission(void);