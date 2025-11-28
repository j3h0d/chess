#include "board_definition.h"
#include "piece_sprites.h"

// Global board state
uint8_t board[BOARD_TILES][BOARD_TILES];

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

// draw the board istelf
void draw_board_tiles() {
    volatile uint8_t *fb = get_framebuffer();

    for (int y = 0; y < BOARD_SIZE; y++) {
        int tile_y = y / TILE_SIZE;
        for (int x = 0; x < BOARD_SIZE; x++) {
            int tile_x = x / TILE_SIZE;

            uint8_t color = ((tile_x + tile_y) & 1) ? COLOR_DARK_TILE : COLOR_LIGHT_TILE;

            int px = BOARD_X0 + x;
            int py = BOARD_Y0 + y;
            fb[py * VIDEO_PITCH + px] = color;
        }
    }
}

// draw the pieces on the board
void draw_piece_at(int file, int rank_board, uint8_t piece) {
    if (piece == PIECE_EMPTY) return;

    volatile uint8_t *fb = get_framebuffer();

    int tile_x0 = BOARD_X0 + file * TILE_SIZE;
    int tile_y0 = BOARD_Y0 + rank_board * TILE_SIZE;

    //get chess piece pointer for this piece
    const uint8_t *sprite = get_piece_sprite(piece);
    if (!sprite) return;

    //draw each shi in right place
    int px0 = tile_x0 + (TILE_SIZE - PIECE_SPRITE_W)  / 2;
    int py0 = tile_y0 + (TILE_SIZE - PIECE_SPRITE_H) / 2;

    for (int sy = 0; sy < PIECE_SPRITE_H; ++sy) {
        int py = py0 + sy;
        if ((unsigned)py >= VIDEO_HEIGHT) continue;

        for (int sx = 0; sx < PIECE_SPRITE_W; ++sx) {
            int px = px0 + sx;
            if ((unsigned)px >= VIDEO_WIDTH) continue;

            // Row-major indexing into 1D array
            uint8_t color = sprite[sy * PIECE_SPRITE_W + sx];

            // Skip transparent background pixels
            if (color == COLOR_TRANSPARENT_KEY) {
                continue;
            }

            fb[py * VIDEO_PITCH + px] = color;
        }
    }
}

// Rita om bräde + pjäser
void redraw_board_and_pieces(void) {
    draw_board_tiles();

    for (int r = 0; r < BOARD_TILES; r++) {
        for (int f = 0; f < BOARD_TILES; f++) {
            uint8_t piece = board[r][f];
            if (piece != PIECE_EMPTY) {
                draw_piece_at(f, r, piece);
            }
        }
    }
}

// ===== Selector and switch decoding =====

void select_tile(int file, int rank_board) {
    volatile uint8_t *fb = get_framebuffer();

    int x0 = BOARD_X0 + file * TILE_SIZE;
    int y0 = BOARD_Y0 + rank_board * TILE_SIZE;
    int x1 = x0 + TILE_SIZE - 1;
    int y1 = y0 + TILE_SIZE - 1;
    int border = 1;

    if (x0 < 0 || y0 < 0 || x1 >= VIDEO_WIDTH || y1 >= VIDEO_HEIGHT) return;

    for (int b = 0; b < border; b++) {
        for (int x = x0; x <= x1; x++) {
            fb[(y0 - b) * VIDEO_PITCH + x] = COLOR_RED;
            fb[(y1 - b) * VIDEO_PITCH + x] = COLOR_RED;
        }
        for (int y = y0; y <= y1; y++) {
            fb[y * VIDEO_PITCH + (x0 - b)] = COLOR_RED;
            fb[y * VIDEO_PITCH + (x1 - b)] = COLOR_RED;
        }
    }
}

// read current selector square from switches: file in bits 0..2, rank in bits 3..5
void read_switch_square(int *file, int *rank_sw) {
    int sw = *SW_DATA;
    *file    = sw & 0x7;
    *rank_sw = (sw >> 3) & 0x7;
}