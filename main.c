#include <stdint.h>
#include "board_definition.h"

void handle_interrupt(void) //not used for now
{
}

int main(void) {
    //pointers
    volatile uint8_t *framebuffer = (volatile uint8_t *)VIDEO_FRAMEBUFFER_BASE;
    volatile uint32_t *vga_controll = (volatile uint32_t *)VIDEO_DMA_BASE;

    //test color
    for (int y = 0; y < VIDEO_HEIGHT; y++) {
        for (int x = 0; x < VIDEO_WIDTH; x++) {
            framebuffer[y * VIDEO_PITCH + x] = COLOR_LIGHT_TILE;
        }
    }

    //telling the DMA which buffer to display
    vga_controll[VIDEO_DMA_BACKBUFFER] = (uint32_t)framebuffer;

    vga_controll[VIDEO_DMA_BUFFER] = 0;

    while (1) { }

    return 0;
}
