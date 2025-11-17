#ifndef PIECE_SPRITES_H
#define PIECE_SPRITES_H

#include <stdint.h>
#include "board_definition.h"

#define PIECE_SPRITE_W  30      // size 30 (thats what she said)
#define PIECE_SPRITE_H  30

//magenta background to be removed later
#define COLOR_TRANSPARENT_KEY RGB332(255, 0, 255)

// Returns pointer to sprite data for a given piece, or NULL if none
const uint8_t *get_piece_sprite(uint8_t piece);

#endif