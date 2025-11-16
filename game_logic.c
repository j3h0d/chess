#include "game_logic.h"

extern uint8_t board[BOARD_TILES][BOARD_TILES];

static int on_board(int f, int r) {
    return f >= 0 && f < BOARD_TILES && r >= 0 && r < BOARD_TILES;
}

static int white_piece(uint8_t piece) {
    return piece == PIECE_W_PAWN || piece == PIECE_W_ROOK || piece == PIECE_W_KNIGHT || piece == PIECE_W_BISHOP || piece == PIECE_W_QUEEN || piece == PIECE_W_KING;
}


static int black_piece(uint8_t piece) {
    return piece == PIECE_B_PAWN || piece == PIECE_B_ROOK || piece == PIECE_B_KNIGHT || piece == PIECE_B_BISHOP || piece == PIECE_B_QUEEN || piece == PIECE_B_KING;
}

static int same_color(int first, int second){
    if(first == PIECE_EMPTY || second == PIECE_EMPTY) return 0;
    if(white_piece(first) && white_piece(second)) return 1;
    if(black_piece(first) && black_piece(second)) return 1;
    return 0;
}

// Rule used for checks (clear line or not)
static int line_move_clear(int src_file, int src_rank, int dst_file, int dst_rank){
    int file_step = 0;
    int rank_step = 0;

    if (dst_file > src_file) file_step = 1;
    else if (dst_file < src_file) file_step = -1;

    if (dst_rank > src_rank) rank_step = 1;
    else if (dst_rank < src_rank) rank_step = -1;

    int file = src_file + file_step;
    int rank = src_rank + rank_step;

    while (file != dst_file || rank != dst_rank) {
        if (!on_board(file, rank)) return 0;
        if (board[rank][file] != PIECE_EMPTY) return 0;
        file += file_step;
        rank += rank_step;
    }
    return 1;
}



// Movement check for rook
static int rook_move(int src_file, int src_rank, int dst_file, int dst_rank){
    if(src_file != dst_file && src_rank != dst_rank) return 0;
    if(!line_move_clear(src_file, src_rank, dst_file, dst_rank)) return 0;
    return 1;
}

// Movement check for bishop
static int bishop_move(int src_file, int src_rank, int dst_file, int dst_rank){
    int dst_x = dst_file - src_file;
    int dst_y = dst_rank - src_rank;

    if(dst_x < 0) dst_x = -dst_x; 
    if(dst_y < 0) dst_y = -dst_y; 

    if(dst_x != dst_y) return 0;
    if(!line_move_clear(src_file, src_rank, dst_file, dst_rank)) return 0;

    return 1;
}

// Movement check for queen by using moves for rook and bishop
static int queen_move(int src_file, int src_rank, int dst_file, int dst_rank){
    int dst_x = dst_file - src_file;
    int dst_y = dst_rank - src_rank;

    if(dst_x < 0) dst_x = -dst_x; 
    if(dst_y < 0) dst_y = -dst_y; 

    if (src_file == dst_file || src_rank == dst_rank) {
        return rook_move(src_file, src_rank, dst_file, dst_rank);
    } else if (dst_x == dst_y) {
        return bishop_move(src_file, src_rank, dst_file, dst_rank);
    } else {
        return 0;
    }
}

// Movemnt for kiight
static int knight_move(int src_file, int src_rank, int dst_file, int dst_rank){
    int dst_x = dst_file - src_file;
    int dst_y = dst_rank - src_rank;

    if(dst_x < 0) dst_x = -dst_x; 
    if(dst_y < 0) dst_y = -dst_y; 
    if ((dst_x == 1 && dst_y == 2) || (dst_x == 2 && dst_y == 1)) return 1;
    return 0;
}

// Movement for king
static int king_move(int src_file, int src_rank, int dst_file,  int dst_rank){
    int dst_x = dst_file - src_file;
    int dst_y = dst_rank - src_rank;

    if(dst_x < 0) dst_x = -dst_x;
    if(dst_y < 0) dst_y = -dst_y;

    if(dst_x <= 1 && dst_y <= 1 && (dst_x != 0 || dst_y != 0)) return 1;
    return 0;
}

// Movement for pawn
static int pawn_move(int src_file, int src_rank, int dst_file, int dst_rank, uint8_t piece){
    int dst_x = dst_file - src_file;
    int dst_y = dst_rank - src_rank;

    int dir; // pawns can move two ways

    if (white_piece(piece)) dir = -1;
    else dir = 1;

    uint8_t dst_piece = board[dst_rank][dst_file];

    // Movement for pawn
    if (dst_x == 0 && dst_y == dir) {
        if (dst_piece == PIECE_EMPTY) return 1;
        return 0;
    }

    // Capture pieces diagonally
    if ((dst_x == 1 || dst_x == -1) && dst_y == dir) {
        if (dst_piece != PIECE_EMPTY && !same_color(piece, dst_piece)) {
            return 1;
        }
        return 0;
    }

    return 0;
}

// Check if move is legal
int legal_move(int src_file, int src_rank, int dst_file, int dst_rank, uint8_t piece, int white_move) {

    // basic validation
    if (!on_board(src_file, src_rank)) return 0;
    if (!on_board(dst_file, dst_rank)) return 0;
    if (src_file == dst_file && src_rank == dst_rank) return 0;

    uint8_t dst_piece = board[dst_rank][dst_file];

    // cannot capture own piece
    if (same_color(piece, dst_piece)) return 0;

    // enforce turn order
    if (white_move && !white_piece(piece)) return 0;
    if (!white_move && !black_piece(piece)) return 0;

    // movement rules for all prices (black and whiet)
    switch (piece) {
        case PIECE_W_ROOK:
        case PIECE_B_ROOK:
            return rook_move(src_file, src_rank, dst_file, dst_rank);

        case PIECE_W_BISHOP:
        case PIECE_B_BISHOP:
            return bishop_move(src_file, src_rank, dst_file, dst_rank);

        case PIECE_W_QUEEN:
        case PIECE_B_QUEEN:
            return queen_move(src_file, src_rank, dst_file, dst_rank);

        case PIECE_W_KNIGHT:
        case PIECE_B_KNIGHT:
            if (!knight_move(src_file, src_rank, dst_file, dst_rank)) return 0;
            if (dst_piece != PIECE_EMPTY && same_color(piece, dst_piece)) return 0;
            return 1;

        case PIECE_W_KING:
        case PIECE_B_KING:
            if (!king_move(src_file, src_rank, dst_file, dst_rank)) return 0;
            if (dst_piece != PIECE_EMPTY && same_color(piece, dst_piece)) return 0;
            return 1;

        case PIECE_W_PAWN:
        case PIECE_B_PAWN:
            return pawn_move(src_file, src_rank, dst_file, dst_rank, piece);

        default:
            return 0;
    }
}
