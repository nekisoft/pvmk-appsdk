//nvmsave.h
//NVM compressed savegame functions
//Bryan E. Topp <betopp@betopp.com> 2024
#ifndef NVMSAVE_H
#define NVMSAVE_H

#include "doomdef.h"
#include "d_player.h"

//Encodes an NVM save into the given 16-byte buffer.
void nvmsave_encode(unsigned char *out16, const player_t *player_state, int episode, int map, int skill);

//Decodes an NVM save from the given 16-byte buffer.
//Returns 0 on success or a negative error number.
int nvmsave_decode(const unsigned char *in16, player_t *player_state, int *episode, int *map, int *skill);


#endif //NVMSAVE_H

