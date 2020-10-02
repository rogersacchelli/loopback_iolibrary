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
#include <stdlib.h>
#include <math.h>

#include "usart.h"
#include "wizchip_conf.h"
#include "w5500.h"
#include "socket.h"
#include "spi.h"
//#include "global.h"


#define DEBUG_ENABLE	1

// ********************************************************************************
// Global Variables
// ********************************************************************************
static FILE mystdout = FDEV_SETUP_STREAM(USART_putchar_printf, NULL, _FDEV_SETUP_WRITE);

uint8_t SPI_w5500_rx_byte(uint16_t addr, uint8_t ctrl);
void w5500_set_nic(wiz_NetInfo net_data);
void w5500_get_nic(wiz_NetInfo net_data);
void w5500_udp_init(uint8_t socket_n, uint16_t udp_port_n);
void SPI_w5500_tx_byte(uint32_t addr, uint8_t* data, uint8_t data_len);
uint8_t SPI_w5500_rx_b(uint32_t addr);

ISR (PCINT2_vect){
	printf("INTERRUPTION ACTIVE FOR PCINT2\r\n");
	PORTB |= (1 << PB7);
}


int main(void)
{
    /* Replace with your application code */
	
	// setup our stdio stream
	stdout = &mystdout;
	
	// fire up the usart
	USART_init ( MYUBRR );
	
	/* Configure Interrupt on PK0
	DDRK |= (0 << PK0);		// PIN A8
	PORTK |= (1 << PK0);	// Set interrupt ports to high level
	PCMSK2 = 0x01;
	PCICR = 0x04;// Enable PCICR
	EICRB |= (0 << ISC40) | (1 << ISC41); // EICRB
	*/
	SPI_MasterInit();
	
	// Enable All Interruptions
	sei();
	
	// Try to read a register from W5500
	uint16_t w5500_spi_addr = 0x0039;
	uint8_t	w5500_spi_ctrl = 0x00;
	
	/*
	
	printf("ADDR PHASE TO SEND IS %lx\r\n", (io_data >> 16));
	printf("CTRL PHASE TO SEND IS %lx\r\n", (io_data & 0x0000FF00) >> 8);
	printf("DATA PHASE TO SEND IS %lx\r\n", (io_data & 0x000000FF) >> 1);
	
	uint8_t chip_version = WIZCHIP_READ(0x0039);
	printf("WIZCHIP version: %u", chip_version);
	
	*/
	
	printf("DATA READ FROM W5500: %u\r\n", (SPI_w5500_rx_byte(w5500_spi_addr,w5500_spi_ctrl)));
	
	wiz_NetInfo wiz_net_info = {.mac  = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}, // Mac address
	.ip   = {192, 168, 0, 55},         // IP address
	.sn   = {255, 255, 255, 0},         // Subnet mask
	.dns =  {8,8,8,8},			  // DNS address (google dns)
	.gw   = {192, 168, 0, 1}, // Gateway address
	.dhcp = NETINFO_STATIC};    //Static IP configuration
	
	w5500_set_nic(wiz_net_info);
	
	w5500_get_nic(wiz_net_info);
	
	// UDP Initialization
	w5500_udp_init(0,4000);

    while (1) 
    {
    }
}

uint8_t SPI_w5500_rx_byte(uint16_t addr, uint8_t ctrl){
	
	uint8_t spi_vector[3];
	uint8_t rx_data;
	
	spi_vector[0] = (addr & 0xFF00);
	spi_vector[1] = (addr & 0x00FF);
	spi_vector[2] = ctrl;
	
	// start transmission
	
	SPI_EnableTransmission();
	
	for(uint8_t i=0;i<sizeof(spi_vector);i++){
		SPI_MasterTransmit(spi_vector[i]);
	}
	
	rx_data = SPI_MasterReceive();
	
	SPI_DisableTransmission();
	
	return rx_data;
}

void SPI_w5500_send_byte(uint16_t addr, uint8_t ctrl, uint8_t data){
	
	uint8_t spi_vector[4];
	spi_vector[0] = (addr & 0xFF00);
	spi_vector[1] = (addr & 0x00FF);
	spi_vector[2] = ctrl;
	spi_vector[3] = data;
	
	// start transmission
	
	SPI_EnableTransmission();
	
	for(uint8_t i=0;i<sizeof(spi_vector);i++){
		SPI_MasterTransmit(spi_vector[i]);
		printf("Sending byte %u \r\n",spi_vector[i]);
	}
	
	SPI_DisableTransmission();
}

void SPI_w5500_tx_byte(uint32_t addr, uint8_t* data, uint8_t data_len){
	
	// W instruction to control phase
	
	uint8_t spi_vector[3];
	spi_vector[0] = (addr >> 16);
	spi_vector[1] = ((addr >> 8) & 0xFF);
	spi_vector[2] = (addr & 0xFF);
	
	// Write byte
	spi_vector[2]+=0x04;
	
	
	// start transmission
	
	SPI_EnableTransmission();
		
	for(uint8_t i=0;i<sizeof(spi_vector);i++){
		SPI_MasterTransmit(spi_vector[i]);
		if (DEBUG_ENABLE && i!=2){
			printf("0x%x ", (spi_vector[i]));
		}
		else{
			printf("0x%x ", (spi_vector[i])-0x04);
		}
	}
	
	for(uint8_t i=0;i<data_len;i++){
		SPI_MasterTransmit(data[i]);
		if (DEBUG_ENABLE){printf("0x%x ", (data[i]));}
	}
	
	if (DEBUG_ENABLE){printf("\n\r");}
	SPI_DisableTransmission();
}

uint8_t SPI_w5500_rx_b(uint32_t addr){
	
	// W instruction to control phase
	
	uint8_t spi_vector[3];
	uint8_t rx_data;
	
	spi_vector[0] = (addr >> 16);
	spi_vector[1] = ((addr >> 8) & 0xFF);
	spi_vector[2] = (addr & 0xFF);
	
	// start transmission
	
	SPI_EnableTransmission();
	
	for(uint8_t i=0;i<sizeof(spi_vector);i++){
		SPI_MasterTransmit(spi_vector[i]);
		if (DEBUG_ENABLE){
			printf("0x%x ", (spi_vector[i]));
		}
	}
	
	rx_data = SPI_MasterReceive();
	
	if (DEBUG_ENABLE){printf("\n\r");}
	SPI_DisableTransmission();
	
	return rx_data;
}

void w5500_set_nic(wiz_NetInfo net_data){
	/************************************************************************/
	/* Send IP configuration to W5500                                       */
	/************************************************************************/
	// IP ADDR
	uint16_t addr = (0x0000 + (SIPR >> 8));
	uint8_t ctrl = 0x04;
	printf("\r\n------ IP ADDR: ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.ip);i++){
		SPI_w5500_send_byte((addr+i),ctrl, net_data.ip[i]);
	}
	
	// MAC ADDR
	addr = (0x0000 + (SHAR >> 8));
	printf("\r\n------ MAC ADDR ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.mac);i++){
		SPI_w5500_send_byte((addr+i),ctrl, net_data.mac[i]);
	}
	
	// GW IP ADD
	addr = (0x0000 + (GAR >> 8));
	printf("\r\n------ GW IP ADD ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.gw);i++){
		SPI_w5500_send_byte((addr+i),ctrl, net_data.gw[i]);
	}
	
	// NET MASK
	addr = (0x0000 + (SUBR >> 8));
	printf("\r\n------ NET MASK ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.sn);i++){
		SPI_w5500_send_byte((addr+i),ctrl, net_data.sn[i]);
	}
}

void w5500_get_nic(wiz_NetInfo net_data){
	/************************************************************************/
	/* Send IP configuration to W5500                                       */
	/************************************************************************/
	// IP ADDR
	uint16_t addr = (0x0000 + (SIPR >> 8));
	uint8_t ctrl = 0x00;
	printf("\r\n------ GET IP ADDR: ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.ip);i++){
		printf("%u ",SPI_w5500_rx_byte((addr+i),ctrl));
	}
	
	// MAC ADDR
	addr = (0x0000 + (SHAR >> 8));
	printf("\r\n------ GET MAC ADDR ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.mac);i++){
		printf("%u ",SPI_w5500_rx_byte((addr+i),ctrl));
	}
	
	// GW IP ADD
	addr = (0x0000 + (GAR >> 8));
	printf("\r\n------ GET GW IP ADD ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.gw);i++){
		printf("%u ",SPI_w5500_rx_byte((addr+i),ctrl));
	}
	
	// NET MASK
	addr = (0x0000 + (SUBR >> 8));
	printf("\r\n------ GET NET MASK ------\r\n");
	for(uint8_t i=0;i<sizeof(net_data.sn);i++){
		printf("%u ",SPI_w5500_rx_byte((addr+i),ctrl));
	}
}

void w5500_udp_init(uint8_t socket_n, uint16_t udp_port_n){
	
	if(DEBUG_ENABLE){printf("\r\n------- INITIALIZING UDP ----------\r\n");}
	if(DEBUG_ENABLE){printf("Socket #%d Port #%u\n\r", socket_n, udp_port_n);}
		
	// UDP MODE
	
	uint8_t data = Sn_MR_UDP;
	if(DEBUG_ENABLE){printf("SnMR: %x@0x%x ", data, (Sn_MR(socket_n)));}
	SPI_w5500_tx_byte((Sn_MR(socket_n)),&data,sizeof(data));
	
	// UDP PORT LISTEN
	uint8_t udp_port[] = {(udp_port_n >> 8), (udp_port_n & 0x00FF)};
	
	for (uint8_t i=0;i<sizeof(udp_port_n);i++){
		if(DEBUG_ENABLE){printf("SnPORT: 0x%x@0x%x ", udp_port[i], ((Sn_PORT(socket_n)+i)));}
		SPI_w5500_tx_byte(((Sn_PORT(socket_n)+i)),&udp_port[i],sizeof(udp_port[i]));
		
	}
	
	// OPEN SOCKET
	data = Sn_CR_OPEN;
	if(DEBUG_ENABLE){printf("SnCR: 0x%x@0x%x ", Sn_CR_OPEN,(Sn_CR(socket_n)));}
	SPI_w5500_tx_byte((Sn_CR(socket_n)),&data,sizeof(data));
	
	// CHECK SOCKET STATUS
	if(DEBUG_ENABLE){
		uint8_t socket_status = SPI_w5500_rx_b(Sn_SR(socket_n));
		printf("Sn_SR: %x@0x%x ",socket_status,Sn_SR(0));
	}
}