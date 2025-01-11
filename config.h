// config.h
#ifndef CONFIG_H
#define CONFIG_H

#define F_CPU 16000000UL  // Define aquí la frecuencia del microcontrolador

// USART configuration
#define USART_BAUDRATE  57600
#define UBRR_VALUE      (((F_CPU/(USART_BAUDRATE*16UL)))-1)
#define RX_BUFFER_SIZE  512
#define RX_LINE_SIZE    128

#endif