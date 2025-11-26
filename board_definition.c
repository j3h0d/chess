#include "board_definition.h"

//switch hardware registers
#define SW_BASE 0x04000010
volatile unsigned int *SW_DATA    = (unsigned int *)(SW_BASE + 0x00);
volatile unsigned int *SW_MASK    = (unsigned int *)(SW_BASE + 0x08);
volatile unsigned int *SW_EDGECAP = (unsigned int *)(SW_BASE + 0x0C);

//button hardware register
#define BTN_BASE 0x040000D0
volatile unsigned int *BTN_DATA    = (unsigned int *)(BTN_BASE + 0x00);
volatile unsigned int *BTN_MASK    = (unsigned int *)(BTN_BASE + 0x08);
volatile unsigned int *BTN_EDGECAP = (unsigned int *)(BTN_BASE + 0x0C);

//timer hardare register
#define TIMER_BASE  0x04000020
volatile unsigned int *TMR_STATUS  = (unsigned int *)(TIMER_BASE + 0x00);
volatile unsigned int *TMR_CONTROL = (unsigned int *)(TIMER_BASE + 0x04);
volatile unsigned int *TMR_PERIODL = (unsigned int *)(TIMER_BASE + 0x08);
volatile unsigned int *TMR_PERIODH = (unsigned int *)(TIMER_BASE + 0x0C);

//the ten leds
volatile int *LED = (int *)0x04000000;

//vga helper funcs
volatile uint8_t *get_framebuffer(void) {
    return (volatile uint8_t *)VIDEO_FRAMEBUFFER_BASE;
}

volatile int *get_vga_control(void) {
    return (volatile int *)VIDEO_DMA_BASE;
}