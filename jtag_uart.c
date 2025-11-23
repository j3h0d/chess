#include <stdint.h>
#include "jtag_uart.h"

// base info
#define JTAG_UART_BASE   0x04000040u

#define JTAG_UART_DATA   (*(volatile uint32_t *)(JTAG_UART_BASE + 0))
#define JTAG_UART_CTRL   (*(volatile uint32_t *)(JTAG_UART_BASE + 4))

void jtag_uart_putc(char c){
    while(((JTAG_UART_CTRL >> 16) && 0xFFFFu) == 0u){
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

int jtag_uart_getc_nonblock(void){
    uint32_t v = JTAG_UART_DATA;
    if ((v & (1u << 15)) == 0u) return -1;
    return (int)(v & 0xFFu);
}

char jtag_uart_getc_block(void){
    int ch;
    do {
        ch = jtag_uart_getc_nonblock();
    } while (ch == -1);
    return (char)ch;
}

int jtag_uart_readline(char *buf, int maxlen){
    int len = 0;

    while (1) {
        char c = jtag_uart_getc_block();

        if (c == '\r') continue;
        if (c == '\n') {
            jtag_uart_putc('\r');
            jtag_uart_putc('\n');
            break;
        }
        // echo
        jtag_uart_putc(c);

        if (len < maxlen - 1) {
            buf[len++] = c;
        }
    }

    buf[len] = '\0';
    return len;
}