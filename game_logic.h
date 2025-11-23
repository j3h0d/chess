#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdint.h>
#include "board_definition.h"

int legal_move(int src_file, int src_rank, int dst_file, int dst_rank, uint8_t piece, int white_move);
int king_in_check(int white);
int is_checkmate(int white_to_move);


#endif
