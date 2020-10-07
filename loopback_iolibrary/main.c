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
#include "avr_utils.h"
//#include "global.h"


#define DEBUG_ENABLE	0
#define VERBOSE			1
#define UDP_HEADER_SIZE	8

// ********************************************************************************
// Global Variables
// ********************************************************************************
static FILE mystdout = FDEV_SETUP_STREAM(USART_putchar_printf, NULL, _FDEV_SETUP_WRITE);

//*********Program metrics
const char compile_date[] PROGMEM    = __DATE__;     // Mmm dd
const char compile_time[] PROGMEM    = __TIME__;     // hh:mm:ss
const char str_prog_name[] PROGMEM   = "\r\nAtMega2560 v1.0 UDP Send Receive IP WIZNET_5500 ETHERNET\r\n"; // Program name
const char str_mcu[] PROGMEM = "AT2560";


void w5500_set_nic(wiz_NetInfo net_data);
void w5500_get_nic();
void w5500_udp_init(uint8_t socket_n, uint16_t udp_port_n);
void SPI_w5500_tx_byte(uint32_t addr, uint8_t* data, uint8_t data_len);
uint8_t SPI_w5500_rx_byte(uint32_t addr);
void w5500_fetch_data(uint8_t socket_n);
void w5500_udp_status(uint8_t socket_n);
uint16_t w5500_get_rx_rd(uint8_t socket_n);
uint16_t w5500_get_msg_size(uint8_t socket_n);

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
	
	printf(">> Free RAM is: %d bytes\r\n", freeRam());
	printf(">> %s", str_prog_name);//
	printf(">> Compiled at: %s %s\r\n", compile_time, compile_date);//
	printf(">> MCU is: %s; CLK is: %luHz\r\n", str_mcu, F_CPU);// MCU Name && FREQ
	
	SPI_MasterInit();
	
	// Enable All Interruptions
	sei();
	
	wiz_NetInfo wiz_net_info = {.mac  = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}, // Mac address
	.ip   = {192, 168, 0, 55},         // IP address
	.sn   = {255, 255, 255, 0},         // Subnet mask
	.dns =  {8,8,8,8},			  // DNS address (google dns)
	.gw   = {192, 168, 0, 1}, // Gateway address
	.dhcp = NETINFO_STATIC};    //Static IP configuration
	
	w5500_set_nic(wiz_net_info);
	
	w5500_get_nic(wiz_net_info);
	
	// UDP Initialization
	uint8_t socket_n = 0;
	uint16_t udp_port_n = 4000;
	
	w5500_udp_init(socket_n, udp_port_n);
	w5500_udp_status(socket_n);

    while (1)
    {
		// Check if data received
		if (w5500_get_msg_size(socket_n) > 0x0000){
			// Fetch data
			if(DEBUG_ENABLE){printf("--------- PACKET RECEIVED ---------\n\r");}
			w5500_fetch_data(socket_n);
			printf("0x%x\n", w5500_get_rx_rd(socket_n));
			
		}
	}
}



void SPI_w5500_tx_byte(uint32_t addr, uint8_t* data, uint8_t data_len){
	
	// W instruction to control phase
	
	uint8_t spi_vector[3];
	spi_vector[0] = addr >> 16;
	spi_vector[1] = addr >> 8;
	spi_vector[2] = addr & 0xFF;
	
	// Write byte
	//spi_vector[2]+=0x04;
	
	
	// start transmission
	
	SPI_EnableTransmission();
		
	for(uint8_t i=0;i<sizeof(spi_vector);i++){
		SPI_MasterTransmit(spi_vector[i]);
		if (DEBUG_ENABLE && i!=2){
			printf("0x%x ", spi_vector[i]);
		}
		else{
			printf("0x%x ", spi_vector[i]);
		}
	}
	
	for(uint8_t i=0;i<data_len;i++){
		SPI_MasterTransmit(data[i]);
		if (DEBUG_ENABLE){printf("0x%x ", (data[i]));}
	}
	
	if (DEBUG_ENABLE){printf("\n\r");}
	SPI_DisableTransmission();
}

uint8_t SPI_w5500_rx_byte(uint32_t addr){
	
	// W instruction to control phase
	
	uint8_t spi_vector[3];
	uint8_t rx_data;
	
	spi_vector[0] = addr >> 16;
	spi_vector[1] = (addr >> 8) & 0xFF;
	spi_vector[2] = addr & 0xFF;
	
	// start transmission
	
	SPI_EnableTransmission();
	
	for(uint8_t i=0;i<sizeof(spi_vector);i++){
		SPI_MasterTransmit(spi_vector[i]);
		if (DEBUG_ENABLE){
			if(DEBUG_ENABLE){printf("0x%x \t|", (spi_vector[i]));}
		}
	}
	
	rx_data = SPI_MasterReceive();
	if(DEBUG_ENABLE){printf("0x%x \t\r\n", rx_data);}
		
	SPI_DisableTransmission();
	
	return rx_data;
}

void w5500_set_nic(wiz_NetInfo net_data){
	/************************************************************************/
	/* Send IP configuration to W5500                                       */
	/************************************************************************/
	// IP ADDR
	printf("\r\n------ IP ADDR: ------\r\n");
	for(uint16_t i=0;i<sizeof(net_data.ip);i++){
		SPI_w5500_tx_byte(SIPR + _W5500_SPI_WRITE_ + (i << 8), &net_data.ip[i],sizeof(net_data.ip[i]));
	}
	
	// MAC ADDR
	printf("\r\n------ MAC ADDR ------\r\n");
	for(uint16_t i=0;i<sizeof(net_data.mac);i++){
		SPI_w5500_tx_byte(SHAR + _W5500_SPI_WRITE_ + (i << 8), &net_data.mac[i],sizeof(net_data.mac[i]));
	}
	
	// GW IP ADD
	printf("\r\n------ GW IP ADD ------\r\n");
	for(uint16_t i=0;i<sizeof(net_data.gw);i++){
		SPI_w5500_tx_byte(GAR + _W5500_SPI_WRITE_ + (i << 8), &net_data.gw[i],sizeof(net_data.gw[i]));
	}
	
	// NET MASK
	printf("\r\n------ NET MASK ------\r\n");
	for(uint16_t i=0;i<sizeof(net_data.sn);i++){
		SPI_w5500_tx_byte(SUBR + _W5500_SPI_WRITE_ + (i << 8), &net_data.sn[i],sizeof(net_data.sn[i]));

	}
}

void w5500_get_nic(){
	/************************************************************************/
	/* Get IP configuration to W5500                                       */
	/************************************************************************/
	
	uint8_t ip_addr[4];
	uint8_t mac_addr[6];
	uint8_t gw_addr[4];
	uint8_t net_mask[4]; 
	
	// IP ADDR
	printf("\r\n------ GET IP ADDR: ------\r\n");
	for(uint16_t i=0;i<sizeof(ip_addr);i++){
		ip_addr[i] = SPI_w5500_rx_byte(SIPR + (i << 8));
	}
	
	// MAC ADDR
	printf("\r\n------ GET MAC ADDR ------\r\n");
	for(uint16_t i=0;i<sizeof(mac_addr);i++){
		mac_addr[i] = SPI_w5500_rx_byte(SHAR + (i << 8));
	}
	
	// GW IP ADD
	printf("\r\n------ GET GW IP ADD ------\r\n");
	for(uint16_t i=0;i<sizeof(gw_addr);i++){
		gw_addr[i] = SPI_w5500_rx_byte(GAR + (i << 8));
	}
	
	// NET MASK
	printf("\r\n------ GET NET MASK ------\r\n");
	for(uint16_t i=0;i<sizeof(net_mask);i++){
		net_mask[i] = SPI_w5500_rx_byte(SUBR + (i << 8));
	}
	
	if(DEBUG_ENABLE){
		printf("-------- NIC CONFIGURATION -------\r\n");
		printf("IP ADD: %u.%u.%u.%u\n", ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3]);
		printf("IP GW: %u.%u.%u.%u\n", gw_addr[0],gw_addr[1],gw_addr[2],gw_addr[3]);
		printf("IP MASK: %u.%u.%u.%u\n", net_mask[0],net_mask[1],net_mask[2],net_mask[3]);
		printf("HW ADDR: %u:%u:%u:%u:%u:%u\n", mac_addr[0],mac_addr[1],mac_addr[2],mac_addr[3],mac_addr[4],mac_addr[5]);
	}
	
	//@TODO: return struct
	
}

void w5500_udp_init(uint8_t socket_n, uint16_t udp_port_n){
	
	if(VERBOSE){printf("\r\n------- INITIALIZING UDP ----------\r\n");}
	if(VERBOSE){printf("Socket #%d Port #%u\n\r", socket_n, udp_port_n);}
		
	uint8_t data = 2;	
		
	// BUFFER ALLOCATION
	if(VERBOSE){printf("Sn_RXBUF_SIZE: %x@0x%x ", data, (Sn_RXBUF_SIZE(socket_n)));}
	SPI_w5500_tx_byte((Sn_RXBUF_SIZE(socket_n)+_W5500_SPI_WRITE_),&data,sizeof(data));
	if(VERBOSE){printf("Sn_TXBUF_SIZE: %x@0x%x ", data, (Sn_TXBUF_SIZE(socket_n)));}
	SPI_w5500_tx_byte((Sn_TXBUF_SIZE(socket_n)+_W5500_SPI_WRITE_),&data,sizeof(data));
		
		
	// UDP MODE
	
	data = Sn_MR_UDP;
	if(VERBOSE){printf("SnMR: %x@0x%x ", data, (Sn_MR(socket_n)));}
	SPI_w5500_tx_byte((Sn_MR(socket_n)+_W5500_SPI_WRITE_),&data,sizeof(data));
	
	// UDP PORT LISTEN
	uint8_t udp_port[2];
	udp_port[0] = udp_port_n >> 8;
	udp_port[1] = udp_port_n & 0x00FF;
	
	if(VERBOSE){printf("SnPORT0: 0x%x@0x%x ", udp_port[0], ((Sn_PORT(socket_n))));}
	SPI_w5500_tx_byte((Sn_PORT(socket_n)+_W5500_SPI_WRITE_),&udp_port[0],sizeof(udp_port[0]));
	if(VERBOSE){printf("SnPORT1: 0x%x@0x%x ", udp_port[1], ((Sn_PORT(socket_n))+1));}
	SPI_w5500_tx_byte((Sn_PORT(socket_n)+_W5500_SPI_WRITE_)+0x100,&udp_port[1],sizeof(udp_port[1]));
	
	// OPEN SOCKET
	data = Sn_CR_OPEN;
	if(VERBOSE){printf("SnCR: 0x%x@0x%x ", Sn_CR_OPEN,(Sn_CR(socket_n)));}
	SPI_w5500_tx_byte((Sn_CR(socket_n)+_W5500_SPI_WRITE_),&data,sizeof(data));
	
	// CHECK SOCKET STATUS
	if(VERBOSE){
		uint8_t socket_status = SPI_w5500_rx_byte(Sn_SR(socket_n));
		printf("Sn_SR: %x@0x%x \n",socket_status,Sn_SR(0));
	}
}

void w5500_udp_status(uint8_t socket_n){
	
	if(VERBOSE){printf("\r\n------- UDP STATUS ----------\r\n");}
	uint8_t socket_status;
	
	if(VERBOSE){
		socket_status = SPI_w5500_rx_byte(Sn_SR(socket_n));
		printf("Sn_SR: %x@0x%x \n",socket_status,Sn_SR(0));
	}
	
	if(VERBOSE){
		socket_status = SPI_w5500_rx_byte(Sn_PORT(socket_n));
		printf("Sn_PORT0: %x@0x%x \n",socket_status,Sn_PORT(socket_n));
		socket_status = SPI_w5500_rx_byte(Sn_PORT(socket_n)+0x100);
		printf("Sn_PORT1: %x@0x%x \n",socket_status,Sn_PORT(socket_n)+0x100);
	}
}


void w5500_fetch_data(uint8_t socket_n){
	if(VERBOSE){printf("\r\n---------- FETCH REGISTER DATA ----------\r\n");}
	if(VERBOSE){printf("Socket #%u\n\r", socket_n);}
		
	// Get Offset Addr
	uint16_t src_ptr;
	uint16_t rcv_size;
	uint32_t addr;
	
	src_ptr = ((SPI_w5500_rx_byte(Sn_RX_RD(socket_n)) << 8) + SPI_w5500_rx_byte(Sn_RX_RD(socket_n) + 0x100));

	if(VERBOSE){printf("Sn_RX_RD: 0x%x@0x%x\n\r", src_ptr, Sn_RX_RD(socket_n));}
		
	rcv_size = (SPI_w5500_rx_byte(Sn_RX_RSR(socket_n)) << 8) + SPI_w5500_rx_byte(Sn_RX_RSR(socket_n)+0x100);
	if(VERBOSE){printf("Sn_RX_RSR: 0x%x@0x%x\n\r", rcv_size, (Sn_RX_RSR(socket_n)));}
		
	// Control Offset Addr
	//uint16_t cntl_byte = (SPI_w5500_rx_byte(Sn_RX_WR(socket_n)) << 8) + SPI_w5500_rx_byte(Sn_RX_WR(socket_n)+0x100);
	//if(DEBUG_ENABLE){printf("Sn_RX_WR: 0x%x@0x%x \n\r", cntl_byte, (Sn_RX_WR(socket_n)));}

	// HEADER READING
	uint8_t header[UDP_HEADER_SIZE];
	/* copy header_size bytes of get_start_address to header_address */
	
	addr = src_ptr;
	addr = (addr << 8) + (WIZCHIP_RXBUF_BLOCK(socket_n) << 3);
	if(VERBOSE){printf("Header Add: 0x%08lx \n\r", addr);}
		
	for(uint16_t i=0; i<rcv_size; i++){
		//header[i] = W5500_READ(src_ptr, header);
		if (i < UDP_HEADER_SIZE){
			header[i] = SPI_w5500_rx_byte(addr);	
			if(DEBUG_ENABLE){printf("header: %u@0x%lx \n\r", header[i], addr);}
		}else if(i>=UDP_HEADER_SIZE){
			if(VERBOSE){
				if(i==UDP_HEADER_SIZE){printf("\n\r---------- START UDP MSG ----------\n\r");}
				printf("%x ", SPI_w5500_rx_byte(addr));
			}
		}
		addr+=0x100;
	}

	if(VERBOSE){printf("\n\r---------- END UDP MSG ----------\n\r");}
	
	src_ptr+=((rcv_size) & 0xFFFF);
	
	// Set Sn_RX_RD to new pointer
	
	uint8_t data = src_ptr >> 8;
	SPI_w5500_tx_byte((Sn_RX_RD(socket_n)) + _W5500_SPI_WRITE_, &data, sizeof(data));
	if(VERBOSE){printf("Sn_RX_RD0: 0x%x@0x%x\n\r", data, (Sn_RX_RD(socket_n)));}

	data = src_ptr & 0xFF;
	SPI_w5500_tx_byte(Sn_RX_RD(socket_n) + 0x100 + _W5500_SPI_WRITE_, &data, sizeof(data));
	if(VERBOSE){printf("Sn_RX_RD1: 0x%x@0x%x\n\r", data, (Sn_RX_RD(socket_n))+0x100);}

	
	// SET RECV
	data = Sn_CR_RECV;
	if(VERBOSE){printf("\r\nSn_CR to RECV\n\r");}
	SPI_w5500_tx_byte(Sn_CR(socket_n) + _W5500_SPI_WRITE_, &data, (sizeof(data)));
	
}
	
uint16_t w5500_get_rx_rd(uint8_t socket_n){
	return (SPI_w5500_rx_byte(Sn_RX_RD(socket_n)) << 8) + SPI_w5500_rx_byte(Sn_RX_RD(socket_n) + 0x100);
}

uint16_t w5500_get_msg_size(uint8_t socket_n){
	return (SPI_w5500_rx_byte(Sn_RX_RSR(socket_n)) << 8) + SPI_w5500_rx_byte(Sn_RX_RSR(socket_n)+0x100);
}