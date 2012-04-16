#include <types.h>
#include <common.h>
#include <io.h>
#include <uart.h>

	
#ifndef UART_DEBUG_PORT
#define UART_DEBUG_PORT		1	//3	//0
#endif

#ifndef UART_DEBUG_BAUD
#define UART_DEBUG_BAUD		(115200)
#endif

int serial_tstc(void)
{
	return uart_tstc(UART_DEBUG_PORT);
}

char serial_getc(void)
{
	return (char)uart_getc(UART_DEBUG_PORT);
}

void serial_putc(char ch)
{
	if(ch=='\n')
		serial_putc('\r');
	
	uart_putc(UART_DEBUG_PORT, ch);
}

void serial_puts(const char* s)
{
	while (*s)
		serial_putc(*s++);
}

int serial_init(void)
{
	return uart_init(UART_DEBUG_PORT, 115200, 'n', 8, 1, 'n');
}
