#include <stdint.h>
#include "board_definition.h"

void handle_interrupt(void)
{
}

int main(void) {
    volatile uint8_t  *framebuffer  = (volatile uint8_t *)VIDEO_FRAMEBUFFER_BASE;
    volatile uint32_t *vga_control  = (volatile uint32_t *)VIDEO_DMA_BASE;

    // clear entire screen to background color
    for (int y = 0; y < VIDEO_HEIGHT; y++) {
        for (int x = 0; x < VIDEO_WIDTH; x++) {
            framebuffer[y * VIDEO_PITCH + x] = COLOR_BACKGROUND;
        }
    }

    // board geometry: 8x8 board, use full height (240 -> tile size 30)
    const int tile_size  = VIDEO_HEIGHT / 8;          // 30
    const int board_size = tile_size * 8;             // 240
    const int board_x0   = (VIDEO_WIDTH - board_size) / 2; // 40
    const int board_y0   = 0;                         // top of screen

    // draw chessboard
    for (int y = board_y0; y < board_y0 + board_size; y++) {
        int tile_y = (y - board_y0) / tile_size;
        for (int x = board_x0; x < board_x0 + board_size; x++) {
            int tile_x = (x - board_x0) / tile_size;

            // light/dark based on tile parity
            uint8_t color =
                ((tile_x + tile_y) & 1) ? COLOR_DARK_TILE : COLOR_LIGHT_TILE;

            framebuffer[y * VIDEO_PITCH + x] = color;
        }
    }

    // tell the DMA which buffer to display
    vga_control[VIDEO_DMA_BACKBUFFER] = (uint32_t)framebuffer;
    vga_control[VIDEO_DMA_BUFFER]     = 0;  // request buffer swap

    while (1) { }

    return 0;
}
