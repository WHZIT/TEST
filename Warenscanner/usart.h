#include <stdint.h>

uint8_t usart_open(uint8_t number);

uint8_t usart_init(uint8_t handle);

uint8_t usart_configure(uint8_t handle, uint32_t bitrate, uint8_t parity, uint8_t stop_bit, uint8_t data_size, uintptr_t *callback_function, uint8_t callback_watermark); //Bei der verwendung des triggers sollte ein timeout vorgesehen werden (evtl. durch hardware)

uint8_t usart_read(uint8_t handle, uint8_t *destination, uint8_t read_length); //Länge der zu lesenden Daten kann angegeben werden

uint8_t usart_write(uint8_t handle, uint8_t *source, uint8_t length); //asynchron realisieren, wird gerade gesendet müsssen die neuen daten angehängt werden

uint8_t usart_close(uint8_t handle);

uint16_t usart_calculate_divider(uint32_t baudrate);

//Available-Funktion