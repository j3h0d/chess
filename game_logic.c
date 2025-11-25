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

// Same color, to check if you can capture a piece
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

    // Movement for pawn (one step)
    if (dst_x == 0 && dst_y == dir) {
        if (dst_piece == PIECE_EMPTY) return 1;
        return 0;
    }

    // two steps movement for pawn only first move 
    if (dst_x == 0 && dst_y == 2*dir) {
        // white starts on rank 6 (row 7), black starts on rank 1 (row 2)
        if (white_piece(piece)) {
            if (src_rank == 6) {
                if (board[src_rank + dir][src_file] == PIECE_EMPTY &&
                    dst_piece == PIECE_EMPTY) //nothing infront
                    return 1;
            }
        } else { //for bakak piece
            if (src_rank == 1) {
                if (board[src_rank + dir][src_file] == PIECE_EMPTY &&
                    dst_piece == PIECE_EMPTY)
                    return 1;
            }
        }
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
// helper: does this piece attack target square (using normal moves)
static int piece_attacks_square(int src_file, int src_rank,
                                int target_file, int target_rank,
                                uint8_t piece) {
    switch (piece) {
        case PIECE_W_ROOK:
        case PIECE_B_ROOK:
            return rook_move(src_file, src_rank, target_file, target_rank);

        case PIECE_W_BISHOP:
        case PIECE_B_BISHOP:
            return bishop_move(src_file, src_rank, target_file, target_rank);

        case PIECE_W_QUEEN:
        case PIECE_B_QUEEN:
            return queen_move(src_file, src_rank, target_file, target_rank);

        case PIECE_W_KNIGHT:
        case PIECE_B_KNIGHT:
            return knight_move(src_file, src_rank, target_file, target_rank);

        case PIECE_W_KING:
        case PIECE_B_KING:
            return king_move(src_file, src_rank, target_file, target_rank);

        case PIECE_W_PAWN:
        case PIECE_B_PAWN:
            return pawn_move(src_file, src_rank, target_file, target_rank, piece);

        default:
            return 0;
    }
}

// used by king_in_check, to see if a square is attacked
static int square_attacked(int target_file, int target_rank, int by_white) {
    for (int r = 0; r < BOARD_TILES; r++) {
        for (int f = 0; f < BOARD_TILES; f++) {
            uint8_t p = board[r][f];
            if (p == PIECE_EMPTY) continue;

            if (by_white) {
                if (!white_piece(p)) continue;
            } else {
                if (!black_piece(p)) continue;
            }

            if (piece_attacks_square(f, r, target_file, target_rank, p)) {
                return 1;
            }
        }
    }
    return 0;
}

// public: check if king of given side is in check (white=1, black=0)
int king_in_check(int white_side) {
    uint8_t king_piece = white_side ? PIECE_W_KING : PIECE_B_KING;
    int king_file = -1;
    int king_rank = -1;

    for (int r = 0; r < BOARD_TILES; r++) {
        for (int f = 0; f < BOARD_TILES; f++) {
            if (board[r][f] == king_piece) {
                king_file = f;
                king_rank = r;
                break;
            }
        }
        if (king_file != -1) break;
    }

    if (king_file == -1) {
        // no king found; treat as "in check" to avoid undefined state
        return 1;
    }

    // check if king square is attacked by opposite side
    return square_attacked(king_file, king_rank, white_side ? 0 : 1);
}

// Check if move is legal (now also prevents own king being in check)
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

    int move_ok = 0;

    // movement rules for all prices (black and whiet)
    switch (piece) {
        case PIECE_W_ROOK:
        case PIECE_B_ROOK:
            move_ok = rook_move(src_file, src_rank, dst_file, dst_rank);
            break;

        case PIECE_W_BISHOP:
        case PIECE_B_BISHOP:
            move_ok = bishop_move(src_file, src_rank, dst_file, dst_rank);
            break;

        case PIECE_W_QUEEN:
        case PIECE_B_QUEEN:
            move_ok = queen_move(src_file, src_rank, dst_file, dst_rank);
            break;

        case PIECE_W_KNIGHT:
        case PIECE_B_KNIGHT:
            if (!knight_move(src_file, src_rank, dst_file, dst_rank)) {
                move_ok = 0;
            } else if (dst_piece != PIECE_EMPTY && same_color(piece, dst_piece)) {
                move_ok = 0;
            } else {
                move_ok = 1;
            }
            break;

        case PIECE_W_KING:
        case PIECE_B_KING:
            if (!king_move(src_file, src_rank, dst_file, dst_rank)) {
                move_ok = 0;
            } else if (dst_piece != PIECE_EMPTY && same_color(piece, dst_piece)) {
                move_ok = 0;
            } else {
                move_ok = 1;
            }
            break;

        case PIECE_W_PAWN:
        case PIECE_B_PAWN:
            move_ok = pawn_move(src_file, src_rank, dst_file, dst_rank, piece);
            break;

        default:
            move_ok = 0;
            break;
    }

    if (!move_ok) return 0;

    // simulate move and check king safety
    uint8_t src_orig = board[src_rank][src_file];
    uint8_t dst_orig = board[dst_rank][dst_file];

    board[src_rank][src_file] = PIECE_EMPTY;
    board[dst_rank][dst_file] = src_orig;

    int in_check = king_in_check(white_move);

    board[src_rank][src_file] = src_orig;
    board[dst_rank][dst_file] = dst_orig;

    if (in_check) return 0; // move illegal because own king is (or stays) in check

    return 1;
}

// helper: does side 'white_to_move' have any legal move?
static int side_has_any_legal_move(int white_to_move) {
    for (int r = 0; r < BOARD_TILES; r++) {
        for (int f = 0; f < BOARD_TILES; f++) {
            uint8_t p = board[r][f];
            if (p == PIECE_EMPTY) continue;

            if (white_to_move) {
                if (!white_piece(p)) continue;
            } else {
                if (!black_piece(p)) continue;
            }

            for (int dst_r = 0; dst_r < BOARD_TILES; dst_r++) {
                for (int dst_f = 0; dst_f < BOARD_TILES; dst_f++) {
                    if (f == dst_f && r == dst_r) continue;
                    if (legal_move(f, r, dst_f, dst_r, p, white_to_move)) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

// public: is side 'white_to_move' currently checkmated?
int is_checkmate(int white_to_move) {
    if (!king_in_check(white_to_move)) return 0;          // must be in check
    if (side_has_any_legal_move(white_to_move)) return 0; // has escape move
    return 1;                                             // in check and no moves
}
