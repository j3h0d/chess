#include <stdint.h>
#include "board_definition.h"

// global board state (rank, file)
uint8_t board[BOARD_TILES][BOARD_TILES];

void handle_interrupt(void) { }

// helpers

static volatile uint8_t  *get_framebuffer(void) { return (volatile uint8_t *)VIDEO_FRAMEBUFFER_BASE; }
static volatile uint32_t *get_vga_control(void) { return (volatile uint32_t *)VIDEO_DMA_BASE; }

static void clear_screen(uint8_t color) {
    volatile uint8_t *fb = get_framebuffer();
    for (int y = 0; y < VIDEO_HEIGHT; y++) {
        for (int x = 0; x < VIDEO_WIDTH; x++) {
            fb[y * VIDEO_PITCH + x] = color;
        }
    }
}

static void draw_board_tiles(void) {
    volatile uint8_t *fb = get_framebuffer();

    for (int y = 0; y < BOARD_SIZE; y++) {
        int tile_y = y / TILE_SIZE;
        for (int x = 0; x < BOARD_SIZE; x++) {
            int tile_x = x / TILE_SIZE;

            uint8_t color =
                ((tile_x + tile_y) & 1) ? COLOR_DARK_TILE : COLOR_LIGHT_TILE;

            int px = BOARD_X0 + x;
            int py = BOARD_Y0 + y;
            fb[py * VIDEO_PITCH + px] = color;
        }
    }
}

// 8x8 glyphs for the letters we need (P, H, B, R, Q, K, ?)
static const uint8_t GLYPH_P[8] = {
    0x7C, 0x44, 0x44, 0x7C, 0x40, 0x40, 0x40, 0x00
};

static const uint8_t GLYPH_H[8] = {
    0x44, 0x44, 0x7C, 0x44, 0x44, 0x44, 0x44, 0x00
};

static const uint8_t GLYPH_B[8] = {
    0x78, 0x44, 0x44, 0x78, 0x44, 0x44, 0x78, 0x00
};

static const uint8_t GLYPH_R[8] = {
    0x7C, 0x44, 0x44, 0x7C, 0x48, 0x44, 0x42, 0x00
};

static const uint8_t GLYPH_Q[8] = {
    0x38, 0x44, 0x44, 0x44, 0x44, 0x4C, 0x3A, 0x00
};

static const uint8_t GLYPH_K[8] = {
    0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 0x44, 0x00
};

static const uint8_t GLYPH_QUESTION[8] = {
    0x3C, 0x42, 0x04, 0x08, 0x10, 0x00, 0x10, 0x00
};

// map PIECE_* value -> letter to display
static char define_piece_letter(uint8_t value) {
    switch (value) {
        case PIECE_W_PAWN:
        case PIECE_B_PAWN:
            return 'P';

        case PIECE_W_KNIGHT:
        case PIECE_B_KNIGHT:
            return 'H';   // horse

        case PIECE_W_BISHOP:
        case PIECE_B_BISHOP:
            return 'B';

        case PIECE_W_ROOK:
        case PIECE_B_ROOK:
            return 'R';

        case PIECE_W_QUEEN:
        case PIECE_B_QUEEN:
            return 'Q';

        case PIECE_W_KING:
        case PIECE_B_KING:
            return 'K';

        default:
            return '?';
    }
}

// draw a single 8x8 character at (x, y)
static void draw_char8x8(int x, int y, char c, uint8_t color) {
    volatile uint8_t *fb = get_framebuffer();
    const uint8_t *glyph = GLYPH_QUESTION;

    switch (c) {
        case 'P': glyph = GLYPH_P; break;
        case 'H': glyph = GLYPH_H; break;
        case 'B': glyph = GLYPH_B; break;
        case 'R': glyph = GLYPH_R; break;
        case 'Q': glyph = GLYPH_Q; break;
        case 'K': glyph = GLYPH_K; break;
        default:  glyph = GLYPH_QUESTION; break;
    }

    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1u << (7 - col))) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < VIDEO_WIDTH &&
                    py >= 0 && py < VIDEO_HEIGHT) {
                    fb[py * VIDEO_PITCH + px] = color;
                }
            }
        }
    }
}


static void draw_piece_at(int file, int rank, uint8_t piece_color, uint8_t piece_value) {
    volatile uint8_t *fb = get_framebuffer();

    int x0 = BOARD_X0 + file * TILE_SIZE;
    int y0 = BOARD_Y0 + rank * TILE_SIZE;

    // size of inner square for the piece
    int s = TILE_SIZE / 2;
    int half = s / 2;

    int x1 = x0 + TILE_SIZE / 2 - half;
    int x2 = x0 + TILE_SIZE / 2 + half;
    int y1 = y0 + TILE_SIZE / 2 - half;
    int y2 = y0 + TILE_SIZE / 2 + half;

    // filled square (piece body)
    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            fb[y * VIDEO_PITCH + x] = piece_color;
        }
    }

    // choose letter for this piece
    char letter = define_piece_letter(piece_value);

    // choose a text color with contrast to the piece body
    uint8_t text_color =
        (piece_color == COLOR_BLACK_PIECE) ? COLOR_LIGHT_TILE : COLOR_DARK_TILE;

    // center 8x8 character in the tile
    int text_x = x0 + (TILE_SIZE - 8) / 2;
    int text_y = y0 + (TILE_SIZE - 8) / 2;

    draw_char8x8(text_x, text_y, letter, text_color);
}

static void draw_all_pieces(void) {
    for (int rank = 0; rank < BOARD_TILES; rank++) {
        for (int file = 0; file < BOARD_TILES; file++) {
            uint8_t p = board[rank][file];
            if (p == PIECE_EMPTY) continue;

            uint8_t color;
            if (p >= PIECE_B_PAWN) {
                color = COLOR_BLACK_PIECE;
            } else {
                color = COLOR_WHITE_PIECE;
            }

            draw_piece_at(file, rank, color, p);
        }
    }
}

static void init_start_position(void) {
    // clear board
    for (int r = 0; r < BOARD_TILES; r++) {
        for (int f = 0; f < BOARD_TILES; f++) {
            board[r][f] = PIECE_EMPTY;
        }
    }

    // black
    board[0][0] = PIECE_B_ROOK;
    board[0][1] = PIECE_B_KNIGHT;
    board[0][2] = PIECE_B_BISHOP;
    board[0][3] = PIECE_B_QUEEN;
    board[0][4] = PIECE_B_KING;
    board[0][5] = PIECE_B_BISHOP;
    board[0][6] = PIECE_B_KNIGHT;
    board[0][7] = PIECE_B_ROOK;

    for (int f = 0; f < BOARD_TILES; f++) {
        board[1][f] = PIECE_B_PAWN;
    }

    // white
    for (int f = 0; f < BOARD_TILES; f++) {
        board[6][f] = PIECE_W_PAWN;
    }

    board[7][0] = PIECE_W_ROOK;
    board[7][1] = PIECE_W_KNIGHT;
    board[7][2] = PIECE_W_BISHOP;
    board[7][3] = PIECE_W_QUEEN;
    board[7][4] = PIECE_W_KING;
    board[7][5] = PIECE_W_BISHOP;
    board[7][6] = PIECE_W_KNIGHT;
    board[7][7] = PIECE_W_ROOK;
}

// redraw after any board[] change
void redraw_board_and_pieces(void) {
    clear_screen(COLOR_BACKGROUND);
    draw_board_tiles();
    draw_all_pieces();
}

int main(void) {
    volatile uint8_t  *framebuffer = get_framebuffer();
    volatile uint32_t *vga_control = get_vga_control();

    init_start_position();
    redraw_board_and_pieces();

    vga_control[VIDEO_DMA_BACKBUFFER] = (uint32_t)framebuffer;
    vga_control[VIDEO_DMA_BUFFER]     = 0;

    while (1) { }

    return 0;
}
