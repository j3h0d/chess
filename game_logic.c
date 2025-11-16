#include "game_logic.h"

extern uint8_t board[BOARD_TILES][BOARD_TILES];

static int on_board(int f, int r) {
    return f >= 0 && f < BOARD_TILES && r >= 0 && r < BOARD_TILES;
}

static int is_white_piece(uint8_t piece) {
    return piece == PIECE_W_ROOK || piece == PIECE_W_KNIGHT || piece == PIECE_W_BISHOP || piece == PIECE_W_QUEEN || piece == PIECE_W_KING;
}

static int is_black_piece(uint8_t piece) {
    return piece == PIECE_B_PAWN || piece == PIECE_B_ROOK || piece == PIECE_B_KNIGHT || piece == PIECE_B_BISHOP || piece == PIECE_B_QUEEN || piece == PIECE_B_KING;
}

static int same_color(int first, int second){
    if(first == PIECE_EMPTY || second == PIECE_EMPTY) return;
    if(is_white__piece(first) && is_black_piece(second)) return 1;
    if(is_white__piece(second) && is_black_piece(first)) return 1;
}

static int line_move_clear(int src_file, int src_rank, int dst_file, int dst_rank){
    int dst_file_step = 0;
    int dst_rank_step = 0;

    if(dst_file > src_file) dst_file_step = 1;
    else if(dst_file > src_file) dst_file_step = -1;

    if(dst_rank > src_rank) dst_rank_step = 1;
    else if(dst_rank > src_rank) dst_rank_step = -1;

    int file = src_file + dst_file_step;
    int rank = src_rank + dst_rank_step;

    while(file != dst_file){
        if(!on_board(file, rank)) return 0;
        if(board[file][rank] == PIECE_EMPTY) return 0;
        file += dst_file_step;
        rank += dst_rank_step;
    }
    return 1;
}

