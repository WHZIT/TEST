#include "usart.h"
#include <stdlib.h>

void main(void)
{
	uint8_t usart_handle;
	char usart_string[] ="Testtext \n\r";
	uint8_t receive_array[2];
	
	usart_handle=usart_open(0);
	usart_init(usart_handle);
	usart_configure(usart_handle, 9600, 0, 1, 8, NULL, 0);
	while(1)
	{
		if (usart_read(usart_handle, receive_array, 2) > 0)
		{
			usart_write(usart_handle, (uint8_t*)receive_array, sizeof(receive_array));
		}
	}
}werewrewr