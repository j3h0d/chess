#ifndef BOARD_DEFS_H
#define BOARD_DEFS_H

#include <stdint.h>

// hardware mapping

#define VIDEO_FRAMEBUFFER_BASE 0x08000000u

#define VIDEO_DMA_BASE 0x04000100u

#define VIDEO_DMA_BUFFER 0 //request a buffer swap
#define VIDEO_DMA_BACKBUFFER 1 //next framebuffer to display
#define VIDEO_DMA_STATUS 3 //status

#define VIDEO_WIDTH 320
#define VIDEO_HEIGHT 240
#define VIDEO_PITCH 320 //nums of bytes per row

// colors

#define RGB332(r, g, b)  ( ((r) & 0xE0) | (((g) >> 3) & 0x1C) | ((b) >> 6) )

#define COLOR_LIGHT_TILE   RGB332(220, 220, 200) //light square col to test

#endif
