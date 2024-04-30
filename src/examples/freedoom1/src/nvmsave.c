//nvmsave.c
//NVM compressed savegame functions
//Bryan E. Topp <betopp@betopp.com> 2024
#include "nvmsave.h"

#include <stdint.h>

//Clamps value for making save password
static int clampval(int val, int min, int max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	
	return val;
}


void nvmsave_encode(uint8_t *out16, const player_t *player_state, int episode, int map, int skill)
{
	const player_t *p = player_state;

	//64-bit value for arithmetic encoding of player state
	uint64_t password_first = 0;
	password_first = (password_first * 201) + clampval(p->health,                       0, 200);
	password_first = (password_first * 201) + clampval(p->armorpoints,                  0, 200);
	password_first = (password_first * 401) + clampval(p->ammo[am_clip],                0, 400);
	password_first = (password_first * 101) + clampval(p->ammo[am_shell],               0, 100);
	password_first = (password_first * 601) + clampval(p->ammo[am_cell],                0, 600);
	password_first = (password_first * 101) + clampval(p->ammo[am_misl],                0, 100);
	password_first = (password_first *   2) + clampval(p->backpack,                     0, 1);
	password_first = (password_first *   2) + clampval(p->didsecret,                    0, 1);
	password_first = (password_first *   3) + clampval(p->armortype,                    0, 2);
	password_first = (password_first *   2) + clampval(p->weaponowned[wp_shotgun],      0, 1);
	password_first = (password_first *   2) + clampval(p->weaponowned[wp_chaingun],     0, 1);
	password_first = (password_first *   2) + clampval(p->weaponowned[wp_missile],      0, 1);
	password_first = (password_first *   2) + clampval(p->weaponowned[wp_plasma],       0, 1);
	password_first = (password_first *   2) + clampval(p->weaponowned[wp_bfg],          0, 1);
	password_first = (password_first *   2) + clampval(p->weaponowned[wp_chainsaw],     0, 1);
	password_first = (password_first *   2) + clampval(p->weaponowned[wp_supershotgun], 0, 1);
	password_first = (password_first *  10) + clampval(p->readyweapon,                  0, 9);
	password_first = (password_first *   2) + clampval(p->cheats & CF_GODMODE,          0, 1);

	//8-bit value for next map and difficulty
	uint32_t password_second = 0;
	password_second = clampval((episode * 10) + map, 0, 40); //0 to 31 for doom2, 0 to 38 for doomu
	password_second = (password_second * 5) + clampval(skill, 0, 4);

	out16[ 0] = (password_first  >>  0) & 0xFF;
	out16[ 1] = (password_first  >>  8) & 0xFF;
	out16[ 2] = (password_first  >> 16) & 0xFF;
	out16[ 3] = (password_first  >> 24) & 0xFF;
	out16[ 4] = (password_first  >> 32) & 0xFF;
	out16[ 5] = (password_first  >> 40) & 0xFF;
	out16[ 6] = (password_first  >> 48) & 0xFF;
	out16[ 7] = (password_first  >> 56) & 0xFF;
	out16[ 8] = (password_second >>  0) & 0xFF;
	out16[ 9] = (password_second >>  8) & 0xFF;
	out16[10] = (password_second >> 16) & 0xFF;
	out16[11] = (password_second >> 24) & 0xFF;
	out16[12] = 'S';
	out16[13] = 'A';
	out16[14] = 'V';
	out16[15] = 'E';
}
	
int nvmsave_decode(const uint8_t *in16, player_t *player_state, int *episode, int *map, int *skill)
{
	if(in16[12] != 'S')
		return -1;
	if(in16[13] != 'A')
		return -1;
	if(in16[14] != 'V')
		return -1;
	if(in16[15] != 'E')
		return -1;
	
	uint64_t password_first = 0;
	for(int bb = 0; bb < 8; bb++)
	{
		password_first = password_first << 8;
		password_first |= in16[7 - bb];
	}
	
	if(player_state != NULL)
	{

		
		int godmode                                = password_first % 2;   password_first /= 2;
		player_state->readyweapon                  = password_first % 10;  password_first /= 10;
		player_state->weaponowned[wp_supershotgun] = password_first % 2;   password_first /= 2;
		player_state->weaponowned[wp_chainsaw]     = password_first % 2;   password_first /= 2;
		player_state->weaponowned[wp_bfg]          = password_first % 2;   password_first /= 2;
		player_state->weaponowned[wp_plasma]       = password_first % 2;   password_first /= 2;
		player_state->weaponowned[wp_missile]      = password_first % 2;   password_first /= 2;
		player_state->weaponowned[wp_chaingun]     = password_first % 2;   password_first /= 2;
		player_state->weaponowned[wp_shotgun]      = password_first % 2;   password_first /= 2;
		player_state->armortype                    = password_first % 3;   password_first /= 3;
		player_state->didsecret                    = password_first % 2;   password_first /= 2;
		player_state->backpack                     = password_first % 2;   password_first /= 2;
		player_state->ammo[am_misl]                = password_first % 101; password_first /= 101;
		player_state->ammo[am_cell]                = password_first % 601; password_first /= 601;
		player_state->ammo[am_shell]               = password_first % 101; password_first /= 101;
		player_state->ammo[am_clip]                = password_first % 401; password_first /= 401;
		player_state->armorpoints                  = password_first % 201; password_first /= 201;
		player_state->health                       = password_first % 201; password_first /= 201;
		
		player_state->weaponowned[wp_fist] = 1;
		player_state->weaponowned[wp_pistol] = 1;
		
		player_state->cheats = godmode ? CF_GODMODE : 0;
	}
	
	uint32_t password_second = 0;
	for(int bb = 0; bb < 4; bb++)
	{
		password_second = password_second << 8;
		password_second |= in16[11 - bb];
	}
	
	if(skill != NULL)
		*skill = password_second % 5;
	
	password_second /= 5;
	
	if(map != NULL)
		*map = password_second % 10;
	
	password_second /= 10;
	
	if(episode != NULL)
		*episode = password_second % 4;
	
	return 0;
}
