#include <stdint.h>
#include "board_definition.h"
#include "game_logic.h"

// ===== Global board state =====
uint8_t board[BOARD_TILES][BOARD_TILES];

// ===== MMIO for switches, buttons =====

// Switch PIO base
#define SW_BASE 0x04000010
volatile unsigned int *SW_DATA    = (volatile unsigned int *)(SW_BASE + 0x00);
volatile unsigned int *SW_MASK    = (volatile unsigned int *)(SW_BASE + 0x08);
volatile unsigned int *SW_EDGECAP = (volatile unsigned int *)(SW_BASE + 0x0C);

// Button PIO base
#define BTN_BASE 0x040000D0
volatile unsigned int *BTN_DATA    = (volatile unsigned int *)(BTN_BASE + 0x00);
volatile unsigned int *BTN_MASK    = (volatile unsigned int *)(BTN_BASE + 0x08);
volatile unsigned int *BTN_EDGECAP = (volatile unsigned int *)(BTN_BASE + 0x0C);

#define BTN_BIT (1u << 0)
#define SW_BIT 0x3F

// enabling interupts
extern void enable_interrupt();

// ===== VGA helpers =====
static volatile uint8_t  *get_framebuffer() { return (volatile uint8_t *)VIDEO_FRAMEBUFFER_BASE; }
static volatile int *get_vga_control() { return (volatile int *)VIDEO_DMA_BASE; }

// draw the board istelf
static void draw_board_tiles() {
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

static int is_white_piece(uint8_t piece) {
    return piece == PIECE_W_PAWN || piece == PIECE_W_ROOK   || piece == PIECE_W_KNIGHT || piece == PIECE_W_BISHOP || piece == PIECE_W_QUEEN  || piece == PIECE_W_KING;
}

static int is_black_piece(uint8_t piece) {
    return piece == PIECE_B_PAWN || piece == PIECE_B_ROOK   || piece == PIECE_B_KNIGHT || piece == PIECE_B_BISHOP || piece == PIECE_B_QUEEN  || piece == PIECE_B_KING;
}

// draw the pieces on the board
static void draw_piece_at(int file, int rank_board, uint8_t piece) {
    if (piece == PIECE_EMPTY) return;

    volatile uint8_t *fb = get_framebuffer();

    int tile_x0 = BOARD_X0 + file * TILE_SIZE;
    int tile_y0 = BOARD_Y0 + rank_board * TILE_SIZE;

    int pw = TILE_SIZE / 2;
    int ph = TILE_SIZE / 2;

    int px0 = tile_x0 + (TILE_SIZE - pw) / 2;
    int py0 = tile_y0 + (TILE_SIZE - ph) / 2;
    int px1 = px0 + pw - 1;
    int py1 = py0 + ph - 1;

    uint8_t color;
    if (is_white_piece(piece)) {
        color = COLOR_WHITE_PIECE;
    } else if (is_black_piece(piece)) {
        color = COLOR_BLACK_PIECE;
    } else {
        return;
    }

    for (int y = py0; y <= py1; y++) {
        for (int x = px0; x <= px1; x++) {
            fb[y * VIDEO_PITCH + x] = color;
        }
    }
}

// ===== Board setup =====
static void init_start_position(void) {
    for (int r = 0; r < BOARD_TILES; r++) {
        for (int f = 0; f < BOARD_TILES; f++) {
            board[r][f] = PIECE_EMPTY;
        }
    }

    // Svarta pjäser
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

    // Vita pjäser
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

// draw red border around a board square (file, rank_board: 0=top,7=bottom)
static void select_tile(int file, int rank_board) {
    volatile uint8_t *fb = get_framebuffer();

    int x0 = BOARD_X0 + file * TILE_SIZE;
    int y0 = BOARD_Y0 + rank_board * TILE_SIZE;
    int x1 = x0 + TILE_SIZE - 1;
    int y1 = y0 + TILE_SIZE - 1;

    if (x0 < 0 || y0 < 0 || x1 >= VIDEO_WIDTH || y1 >= VIDEO_HEIGHT) return;

    for (int x = x0; x <= x1; x++) {
        fb[y0 * VIDEO_PITCH + x] = COLOR_RED;
        fb[y1 * VIDEO_PITCH + x] = COLOR_RED;
    }
    for (int y = y0; y <= y1; y++) {
        fb[y * VIDEO_PITCH + x0] = COLOR_RED;
        fb[y * VIDEO_PITCH + x1] = COLOR_RED;
    }
}

// read current selector square from switches: file in bits 0..2, rank in bits 3..5
static void read_switch_square(int *file, int *rank_sw) {
    int sw = *SW_DATA;
    *file    = sw & 0x7;
    *rank_sw = (sw >> 3) & 0x7;
}

/*static int square_has_piece(int file, int rank_board) {
    if (file < 0 || file >= BOARD_TILES) return 0;
    if (rank_board < 0 || rank_board >= BOARD_TILES) return 0;
    return board[rank_board][file] != PIECE_EMPTY;
}*/

static void perform_move(int src_file, int src_rank, int dst_file, int dst_rank){
    uint8_t piece = board[src_rank][src_file];
    if (piece == PIECE_EMPTY) return;

    board[src_rank][src_file] = PIECE_EMPTY;
    board[dst_rank][dst_file] = piece;

    redraw_board_and_pieces();
    select_tile(dst_file, dst_rank);
}

// ===== Interrupt init =====
void labinit(void) {
    // Enable switch
    *SW_MASK    = SW_BIT;
    *SW_EDGECAP = SW_BIT;

    // Enable button
    *BTN_MASK    = BTN_BIT;
    *BTN_EDGECAP = BTN_BIT;

    enable_interrupt();
}

static int sel_file = -1;
static int sel_rank = -1;
static int btn_down = 0;
static int white_to_move = 1;
// ===== Interrupt init =====
void handle_interrupt(unsigned cause) {

    if (cause == 17) {
        *SW_EDGECAP = SW_BIT;

        int f, r_sw;
        read_switch_square(&f, &r_sw);
        int board_rank = 7 - r_sw;

        if (f >= 0 && f < BOARD_TILES && board_rank >= 0 && board_rank < BOARD_TILES) {
            if (f != sel_file || board_rank != sel_rank) {
                redraw_board_and_pieces();
                select_tile(f, board_rank);
                sel_file = f;
                sel_rank = board_rank;
            }
        }
    }

    static int src_file = -1;
    static int src_rank = -1;
    static int piece_selected = 0;
    if (cause == 18) {
        int pending = *BTN_EDGECAP;
        *BTN_EDGECAP = pending;

        unsigned int btn_state = *BTN_DATA;
        if ((btn_state & BTN_BIT) == 0) {
            btn_down = 0;
            return;
        }
        if (btn_down) {
            return;
        }
        btn_down = 1;

        if (sel_file < 0 || sel_file >= BOARD_TILES || sel_rank < 0 || sel_rank >= BOARD_TILES) {
            return;
        }

        if(!piece_selected){
            uint8_t piece = board[sel_rank][sel_file];
            if (piece == PIECE_EMPTY) {
                return;
            }

            if (white_to_move) {
                if (!is_white_piece(piece)) {
                    return;
                }
            } else {
                if (!is_black_piece(piece)) {
                    return;
                }
            }

            src_file = sel_file;
            src_rank = sel_rank;
            piece_selected = 1;

        } else {
            int dst_file = sel_file;
            int dst_rank = sel_rank;

            uint8_t piece = board[src_rank][src_file];
            if (legal_move(src_file, src_rank, dst_file, dst_rank, piece, white_to_move)) {
                perform_move(src_file, src_rank, dst_file, dst_rank);
                piece_selected = 0;
                white_to_move = !white_to_move;
            } else {
                piece_selected = 0;
            }
        }
    }
}

// ===== main =====
int main() {
    volatile uint8_t  *framebuffer = get_framebuffer();
    volatile int *vga_control = get_vga_control();

    labinit();
    init_start_position();
    redraw_board_and_pieces();

    vga_control[VIDEO_DMA_BACKBUFFER] = (int)framebuffer;
    vga_control[VIDEO_DMA_BUFFER]     = 0;

    while (1) {
        // all game interaction happens in handle_interrupt()
    }

    return 0;
}
