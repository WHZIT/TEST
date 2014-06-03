#include "usart.h"
#include "global_defines.h"
#include <stdint.h>
#include <stdlib.h>
#include "LPC17XX.h"



#define USART_NOT_IN_USE				0
#define USART_IN_USE						1
#define PCONP_REGISTER					0x400FC0C4
#define UART0_START_REGISTER		0x4000C000
#define UART1_START_REGISTER		0x40010000
#define UART2_START_REGISTER		0x40098000
#define UART3_START_REGISTER		0x4009C000
#define UART_LSR_OFFSET					14
#define UART_FIFOLVL_OFFSET			58

struct usart_mgmt{uint8_t isInUse; uint8_t handle; uintptr_t start_register;};
struct usart_mgmt usart_array[4];

uint16_t usart_calculate_divider(uint32_t baudrate);

uint8_t usart_open(uint8_t number)
{
	if (number < 4)
	{
		if (usart_array[number].isInUse == USART_NOT_IN_USE)
		{
			usart_array[number].isInUse=USART_IN_USE;
			usart_array[number].handle=number;
			switch(number)
			{
				case	0	:		usart_array[number].start_register=UART0_START_REGISTER;
										break;
				case	1	:		usart_array[number].start_register=UART1_START_REGISTER;
										break;
				case	2	:		usart_array[number].start_register=UART2_START_REGISTER;
										break;
				case	3	:		usart_array[number].start_register=UART3_START_REGISTER;
										break;
			}
			return number+1; //Handle=USART+1
		}
	}
	return RETURN_FAIL;
}

uint8_t usart_init(uint8_t handle)
{
	uintptr_t *PCONP; //Power Control for Peripherals Register
	PCONP=(uintptr_t *)PCONP_REGISTER;
	
	if ( (handle > 0) && (handle < 5) && (usart_array[handle-1].isInUse == USART_IN_USE) )
	{
		switch (handle)
		{
			case 1	:	*PCONP|=(1<<3);
								break;
			case 2	:	*PCONP|=(1<<4);
								break;
			case 3	:	*PCONP|=(1<<24);
								break;
			case 4	:	*PCONP|=(1<<25);
								break;
		}
		return RETURN_SUCCESS;
	}
	return RETURN_FAIL;
		
}

uint8_t usart_configure(uint8_t handle, uint32_t bitrate, uint8_t parity, uint8_t stop_bit, uint8_t data_size, uintptr_t *callback_function, uint8_t callback_watermark)
{
	uint16_t divider;
	uintptr_t *U0LCR;
	uintptr_t *U0DLL;
	uintptr_t *U0DLM;
	uintptr_t *U0FCR;
	uintptr_t *PINSEL0;
	uintptr_t *PINMODE0;
	
	//Implemntierung nur für USART0
	U0LCR=0x4000C00C;
	U0DLL=0x4000C000;
	U0DLM=0x4000C004;
	U0FCR=0x4000C008;
	PINSEL0=0x4002C000;
	PINMODE0=0x4002C040;
	
	if ( (handle > 0) && (handle < 5) && (usart_array[handle-1].isInUse == USART_IN_USE) )
	{
		if ( (parity < 2) && ( (stop_bit > 0) &&  (stop_bit < 3) ) && ( (data_size > 4) && (data_size < 9) ) )
		{
			divider=usart_calculate_divider(bitrate);
			if (divider != 0)
			{
				*U0LCR|=(1<<7); //Aktiviert den Zugriff auf die Divider-Register
				*U0DLL=(divider&0xFF);
				*U0DLM=((divider>>8)&0xFF);
				*U0LCR&=~(1<<7); //Deaktiviert den Zugriff auf die Divider-Register
				switch (data_size)
				{
					case 5	:	*U0LCR&=~(1<<0);
										*U0LCR&=~(1<<1);
										break;
					case 6	:	*U0LCR|=(1<<0);
										*U0LCR&=~(1<<1);
										break;
					case 7	:	*U0LCR&=~(1<<0);
										*U0LCR|=(1<<1);
										break;
					case 8	:	*U0LCR|=(1<<0);
										*U0LCR|=(1<<1);
										break;
				}
				if (stop_bit == 1)
				{
					*U0LCR&=~(1<<2);
				}
				else
				{
					*U0LCR|=(1<<2);
				}
				if (parity == 1)
				{
					*U0LCR|=(1<<3);
				}
				else
				{
					*U0LCR&=~(1<<3);
				}
				*U0FCR|=(1<<0);
				*PINSEL0|=(1<<4); //P0.2=TXD0
				*PINSEL0&=~(1<<5); //P0.2=TXD0
				*PINSEL0|=(1<<6); //P0.3=RXD0
				*PINSEL0&=~(1<<7); //P0.3=RXD0
				*PINMODE0&=~(1<<4); //P0.2 Kein Pull-Up, kein Pull-Down
				*PINMODE0|=(1<<5); //P0.2 Kein Pull-Up, kein Pull-Down
				*PINMODE0&=~(1<<6); //P0.3 Kein Pull-Up, kein Pull-Down
				*PINMODE0|=(1<<7); //P0.3 Kein Pull-Up, kein Pull-Down
				
				return RETURN_SUCCESS;
			}
		}
	}
	return RETURN_FAIL;
}

uint8_t usart_read(uint8_t handle, uint8_t *destination, uint8_t read_length)
{
	uintptr_t *UxLSR;
	uintptr_t *UxFIFOLVL;
	uintptr_t *UxRBR;
	uint8_t rx_fifo_level; //Füllstand des Empfangspuffers
	uint8_t bytes_read; //Anzahl der Bytes die gelesen wurden
	
	bytes_read=0;
	
	if ( (handle > 0) && (handle < 5) && (usart_array[handle-1].isInUse == USART_IN_USE) )
	{
		UxLSR=(uintptr_t *)usart_array[handle-1].start_register+UART_LSR_OFFSET;
		UxFIFOLVL=(uintptr_t *)usart_array[handle-1].start_register+UART_LSR_OFFSET;
		UxRBR=usart_array[handle-1].start_register;
		if ( (read_length > 0) && (read_length < 17) && (destination != NULL) )
		{
			if ( ((*UxLSR)&(1<<0)) == 1)
			{
				rx_fifo_level=*UxFIFOLVL&((1<<0)+(1<<1)+(1<<2));
				while ( ( ((*UxLSR)&(1<<0)) == 1) && (read_length > 0) )
				{
					*destination=*UxRBR;
					destination++;
					bytes_read++;
				}

			}
		}
	}
	return bytes_read;
}

uint8_t usart_write(uint8_t handle, uint8_t *source, uint8_t length)
{
	uintptr_t *U0THR;
	uintptr_t *U0LSR;
	uint8_t i;
	
	//Implementierung nur für USART0
	U0THR=0x4000C000;
	U0LSR=0x4000C014;
	
	if ( (handle > 0) && (handle < 5) && (usart_array[handle-1].isInUse == USART_IN_USE) )
	{
		if ( (source != NULL) && (length > 0) )
		{
			i=0;
			while (length > 0)
			{
				length--;
				while ( (*U0LSR&(1<<5)) == 0)
				{}
				*U0THR=source[i++];
			}
			return RETURN_SUCCESS;
		}
	}
	return RETURN_FAIL;
}

uint8_t usart_close(uint8_t handle)
{
	uintptr_t *PCONP; //Power Control for Peripherals Register
	PCONP=(uintptr_t *)PCONP_REGISTER;
	
	if ( (handle > 0) && (handle < 5) && (usart_array[handle-1].isInUse == USART_IN_USE) )
	{
		switch (handle)
		{
			case 1	:	*PCONP&=~(1<<3);
								break;
			case 2	:	*PCONP&=~(1<<4);
								break;
			case 3	:	*PCONP&=~(1<<24);
								break;
			case 4	:	*PCONP&=~(1<<25);
								break;
		}
		return RETURN_SUCCESS;
	}
	return RETURN_FAIL;
}

uint16_t usart_calculate_divider(uint32_t baudrate)
{
	uint16_t returnvalue;
	if ( baudrate <= 115200 )
	{
		returnvalue=(18000000/(baudrate*16));
		return returnvalue;
	}
	return 0;
}