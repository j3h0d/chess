#ifndef BOARD_DEFS_H
#define BOARD_DEFS_H

#include <stdint.h>

// hardware mapping

#define VIDEO_FRAMEBUFFER_BASE 0x08000000u
#define VIDEO_DMA_BASE         0x04000100u

#define VIDEO_DMA_BUFFER       0  // request a buffer swap
#define VIDEO_DMA_BACKBUFFER   1  // next framebuffer to display
#define VIDEO_DMA_STATUS       3

#define VIDEO_WIDTH            320
#define VIDEO_HEIGHT           240
#define VIDEO_PITCH            320  // bytes per row

// colors in RGB332 (8-bit per pixel)

#define RGB332(r, g, b)  ( ((r) & 0xE0) | (((g) >> 3) & 0x1C) | ((b) >> 6) )

#define COLOR_BACKGROUND  RGB332(100, 100, 100)
#define COLOR_LIGHT_TILE  RGB332(220, 220, 200)
#define COLOR_DARK_TILE   RGB332( 80,  80,  80)
#define COLOR_WHITE_PIECE RGB332(255, 255, 255)
#define COLOR_BLACK_PIECE RGB332(  0,   0,   0)

// board geometry

#define BOARD_TILES  8
#define TILE_SIZE    (VIDEO_HEIGHT / BOARD_TILES)     // 30
#define BOARD_SIZE   (TILE_SIZE * BOARD_TILES)        // 240
#define BOARD_X0     ((VIDEO_WIDTH - BOARD_SIZE) / 2) // 40
#define BOARD_Y0     0

// pieces

typedef enum {
    PIECE_EMPTY = 0,

    PIECE_W_PAWN,
    PIECE_W_KNIGHT,
    PIECE_W_BISHOP,
    PIECE_W_ROOK,
    PIECE_W_QUEEN,
    PIECE_W_KING,

    PIECE_B_PAWN,
    PIECE_B_KNIGHT,
    PIECE_B_BISHOP,
    PIECE_B_ROOK,
    PIECE_B_QUEEN,
    PIECE_B_KING
} Piece;

// global board state

extern uint8_t board[BOARD_TILES][BOARD_TILES];

#endif
