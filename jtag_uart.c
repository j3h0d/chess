#include <stdint.h>
#include "jtag_uart.h"

// base info
#define JTAG_UART_BASE   0x04000040u

#define JTAG_UART_DATA   (*(volatile uint32_t *)(JTAG_UART_BASE + 0))
#define JTAG_UART_CTRL   (*(volatile uint32_t *)(JTAG_UART_BASE + 4))

void jtag_uart_putc(char c){
    while(((JTAG_UART_CTRL >> 16) && 0xFFFF) == 0){
        //
    }
    JTAG_UART_DATA = (uint8_t)c;
}

// Write a null-terminated string
void jtag_uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            
            jtag_uart_putc('\r');
        }
        jtag_uart_putc(*s++);
    }
}