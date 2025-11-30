#include <stdint.h>
#include "board_definition.h"
#include "game_logic.h"
#include "piece_sprites.h"
#include "jtag_uart.h"

static int game_over = 0;
// clock state
static volatile int timeoutcount  = 0;  // timer ticks (10 per second)
static volatile int seconds_left  = 60; // starts at 60 for current player

static int sel_file = -1;
static int sel_rank = -1;
static int btn_down = 0;
static int white_to_move = 1;

//moved from inside handle_interrupt to outside to be independent
static int src_file = -1;
static int src_rank = -1;
static int piece_selected = 0;

// enabling interrupts
extern void enable_interrupt();

//board display helper functions
static void set_leds(int led_mask) {
    volatile int *LED = (volatile int*) 0x04000000;
    *LED = led_mask & 0x3FF; // -lower 10 bits used-
}

static void set_displays(int display_number, int value){
    int addr = 0x04000050 + (0x10 * display_number);
    volatile int *GET_DISPLAY = (volatile int*) addr;

    switch (value) { 
        case 0:*GET_DISPLAY = 0b11000000; break;
        case 1:*GET_DISPLAY = 0b11111001; break;
        case 2:*GET_DISPLAY = 0b10100100; break;
        case 3:*GET_DISPLAY = 0b10110000; break;
        case 4:*GET_DISPLAY = 0b10011001; break;
        case 5:*GET_DISPLAY = 0b10010010; break;
        case 6:*GET_DISPLAY = 0b10000010; break;
        case 7:*GET_DISPLAY = 0b11111000; break;
        case 8:*GET_DISPLAY = 0b10000000; break;
        case 9:*GET_DISPLAY = 0b10010000; break;
        default:*GET_DISPLAY = 0b00000000; break; //all on is defaultbreak;
    }
}

void update_clock_outputs(void) {
    int sec = seconds_left;
    if (sec < 0)  sec = 0;
    if (sec > 99) sec = 99;   // we only show 2 digits

    int tens = sec / 10;
    int ones = sec % 10;
    static volatile int zeros = 0;

    // Rightmost two digits show seconds_left
    set_displays(0, ones);  // rightmost
    set_displays(1, tens);  // next
    // Blank the other four digits
    set_displays(2, zeros);
    set_displays(3, zeros);
    set_displays(4, zeros);
    set_displays(5, zeros);

    // diods logic:
    // >10 seconds -> all 10 leds on
    // N in [0...10] -> N rightmost leds on
    if (seconds_left > 10) {
        set_leds(0x3FF); // 10 ones: 0b11_1111_1111
    } else if (seconds_left >= 0) {
        //between 10s and 0s diods turn off one by one
        unsigned int mask = (seconds_left == 0) ? 0 : ((1u << seconds_left) - 1u);
        set_leds(mask);
    } else {
        set_leds(0);
    }
}

static int is_white_piece(uint8_t piece) {
    return piece == PIECE_W_PAWN || piece == PIECE_W_ROOK || piece == PIECE_W_KNIGHT || piece == PIECE_W_BISHOP || piece == PIECE_W_QUEEN  || piece == PIECE_W_KING;
}

static int is_black_piece(uint8_t piece) {
    return piece == PIECE_B_PAWN || piece == PIECE_B_ROOK || piece == PIECE_B_KNIGHT || piece == PIECE_B_BISHOP || piece == PIECE_B_QUEEN  || piece == PIECE_B_KING;
}

// Board setup
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
    *TMR_STATUS = 0;
    *TMR_PERIODL = 0xC6BF;
    *TMR_PERIODH = 0x002D;
    *TMR_CONTROL = 0x7;

    timeoutcount = 0;
    seconds_left = 60;
    update_clock_outputs();

    // Enable switch
    *SW_MASK    = SW_BIT;
    *SW_EDGECAP = SW_BIT;

    // Enable button
    *BTN_MASK    = BTN_BIT;
    *BTN_EDGECAP = BTN_BIT;

    enable_interrupt();
}

//game status terminal printer
static void uart_print_status(const char *color, const char *type, int file, int rank){
    char cap[40];
    int inc = 0;

    //copy color
    while (*color) cap[inc++] = *color++;

    cap[inc++] = ' ';

    //copy piece type
    while (*type) cap[inc++] = *type++;

    //add "selected at" after white/balak
    const char *text = " selected at ";
    while (*text) cap[inc++] = *text++;

    //file letter
    cap[inc++] = 'a' + file;

    //rank number
    cap[inc++] = '0' + (8 - rank);

    // newline
    cap[inc++] = '\n';
    cap[inc++] = 0;

    jtag_uart_puts(cap);

}

// ===== Interrupt init =====
void handle_interrupt(unsigned cause) {

    if (cause == 16) {
        *TMR_STATUS = 0;
        timeoutcount++;

        if (timeoutcount >= 10) {
            timeoutcount = 0;

            if (seconds_left > 0) {
                seconds_left--;
            }

            if (seconds_left <= 0) {
                seconds_left = 0;
                update_clock_outputs(); //shows 0 and leds all off

                white_to_move = !white_to_move;

                // resets all move selections
                piece_selected = 0;
                src_file = -1;
                src_rank = -1;

                seconds_left = 60;
                timeoutcount = 0;
            }
            update_clock_outputs(); //update status of clock for new player
        }
        return;
    }

    if (cause == 17) {
        *SW_EDGECAP = SW_BIT;

        int f, r_sw;
        read_switch_square(&f, &r_sw);
        int board_rank = 7 - r_sw;

        if (f >= 0 && f < BOARD_TILES && board_rank >= 0 && board_rank < BOARD_TILES) {
            if (f != sel_file || board_rank != sel_rank) {
                redraw_board_and_pieces();

                if (piece_selected) {
                    select_tile(src_file, src_rank);
                }

                select_tile(f, board_rank);
                sel_file = f;
                sel_rank = board_rank;
            }
        }
    }

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
        if (game_over) {
            int pending_done = *BTN_EDGECAP;
            *BTN_EDGECAP = pending_done;
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

            /*printing which chess piece was
            selected at the moment*/
            //{}[]

            char *type; //printing specific piece name thats selected
            if (piece == PIECE_W_PAWN || piece == PIECE_B_PAWN) type = "pawn";
            else if (piece == PIECE_W_ROOK || piece == PIECE_B_ROOK) type = "rook";
            else if (piece == PIECE_W_KNIGHT || piece == PIECE_B_KNIGHT) type = "knight";
            else if (piece == PIECE_W_BISHOP || piece == PIECE_B_BISHOP) type = "bishop";
            else if (piece == PIECE_W_QUEEN || piece == PIECE_B_QUEEN) type = "queen";
            else type = "king";

            uart_print_status(white_to_move ? "White" : "Black",
                            type, sel_file, sel_rank);

            //---

            src_file = sel_file;
            src_rank = sel_rank;
            piece_selected = 1;

            redraw_board_and_pieces();
            select_tile(src_file, src_rank);
            select_tile(sel_file, sel_rank);

        } else {
            int dst_file = sel_file;
            int dst_rank = sel_rank;

            uint8_t piece = board[src_rank][src_file];
            if (legal_move(src_file, src_rank, dst_file, dst_rank, piece, white_to_move)) {
                perform_move(src_file, src_rank, dst_file, dst_rank);
                piece_selected = 0;
                white_to_move = !white_to_move;
                seconds_left = 60;
                timeoutcount = 0;
                update_clock_outputs();

                //printing king status
                if (king_in_check(white_to_move)) {
                    if (white_to_move) jtag_uart_puts("White king is in check!\n");
                    else jtag_uart_puts("Black king is in check!\n");

                    // checkmate?
                    if (is_checkmate(white_to_move)) {
                        if (white_to_move) jtag_uart_puts("Black wins by checkmate!\n");
                        else jtag_uart_puts("White wins by checkmate!\n");
                    }
                }
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

    #ifdef PERF
    benchmark_logic(); //test 1
    benchmark_render(); //test 2
    while (1) {
        // all game interaction happens in handle_interrupt()
    }
    #endif

    return 0;
}