#ifndef JTAG_UART_H
#define JTAG_UART_H

void jtag_uart_putc(char c);    //send char
void jtag_uart_puts(const char *s); // send string

int jtag_uart_getc_nonblock(); // try to read one char (else -1)
char jtag_uart_getc_block();  // wait until char available
int jtag_uart_readline(char *bul, int maxlen); // read line after enter


#endif