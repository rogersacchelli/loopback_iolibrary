// ********************************************************************************
//								UART INITIALIZATION
// ********************************************************************************

#define BAUD 19200
#define MYUBRR F_CPU/16/BAUD-1

// ********************************************************************************
// Function Prototypes
// ********************************************************************************
void USART_init(uint16_t ubrr);
//char usart_getchar( void );
void USART_Transmit( unsigned char data );
unsigned char USART_Receive( void );
void USART_pstr (char *s);
unsigned char USART_kbhit(void);
int USART_putchar_printf(char var, FILE *stream);