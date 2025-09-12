//chanim.c
//Functions for animating characters
//Bryan E. Topp <betopp@betopp.com> 2025

#include "chanim.h"
#include "proj.h"
#include "images.h"
#include "trigfunc.h"

#include <stdbool.h>

//Parts of person graphic
typedef enum chanim_part_idx_e
{
	PA_NONE = 0,
	PA_HEAD,
	PA_SHOULDERS,
	PA_TORSO,
	PA_HIPS,
	PA_LEGL0,
	PA_LEGL1,
	PA_LEGR0,
	PA_LEGR1,
	PA_FOOTL,
	PA_FOOTR,
	PA_ARML0,
	PA_ARML1,
	PA_ARMR0,
	PA_ARMR1,
	PA_SHADOW,
	PA_MAX
} chanim_part_idx_t;
typedef struct chanim_part_s
{
	images_file_t imf; //Image file index
	int vh; //Virtual height, cm 24.8
	//todo - maybe alternates/rotations/whatever
} chanim_part_t;
static const chanim_part_t chanim_part_table[PA_MAX] = 
{
	[PA_HEAD]      = { .imf = IMF_CARD_SPHERE, .vh = 45*256 },
	[PA_SHOULDERS] = { .imf = IMF_CARD_SPHERE, .vh = 18*256 },
	[PA_TORSO]     = { .imf = IMF_CARD_SPHERE, .vh = 48*256 },
	[PA_HIPS]      = { .imf = IMF_CARD_SPHERE, .vh = 32*256 },
	[PA_LEGL0]     = { .imf = IMF_CARD_SPHERE, .vh = 20*256 },
	[PA_LEGL1]     = { .imf = IMF_CARD_SPHERE, .vh = 16*256 },
	[PA_LEGR0]     = { .imf = IMF_CARD_SPHERE, .vh = 20*256 },
	[PA_LEGR1]     = { .imf = IMF_CARD_SPHERE, .vh = 16*256 },
	[PA_FOOTL]     = { .imf = IMF_CARD_SPHERE, .vh = 13*256 },
	[PA_FOOTR]     = { .imf = IMF_CARD_SPHERE, .vh = 13*256 },
	[PA_ARML0]     = { .imf = IMF_CARD_SPHERE, .vh = 18*256 },
	[PA_ARML1]     = { .imf = IMF_CARD_SPHERE, .vh = 14*256 },
	[PA_ARMR0]     = { .imf = IMF_CARD_SPHERE, .vh = 18*256 },
	[PA_ARMR1]     = { .imf = IMF_CARD_SPHERE, .vh = 14*256 },
	[PA_SHADOW]    = { .imf = IMF_CARD_BALLSH, .vh = 20*256 },
};

//Definitions of each animation frame using those parts
typedef struct chanim_s
{
	chanim_part_idx_t rel;
	int32_t pos[3];
} chanim_t;
static chanim_t chanim_table[AN_MAX][PA_MAX] = 
{
	//Standing/idle - just one frame for now
	[AN_STAND] = 
	{
		[PA_HEAD]      = { .rel = PA_SHOULDERS, .pos = {      0,       0,  12*256 } },
		[PA_SHOULDERS] = { .rel = PA_TORSO,     .pos = {      0,       0,  24*256 } },
		[PA_TORSO]     = { .rel = 0,            .pos = {      0,       0,  80*256 } },
		[PA_HIPS]      = { .rel = PA_TORSO,     .pos = {      0,       0, -24*256 } },
		[PA_LEGL0]     = { .rel = PA_HIPS,      .pos = {      0,   8*256,  -8*256 } },
		[PA_LEGL1]     = { .rel = PA_LEGL0,     .pos = {      0,   8*256, -24*256 } },
		[PA_LEGR0]     = { .rel = PA_HIPS,      .pos = {      0,  -8*256,  -8*256 } },
		[PA_LEGR1]     = { .rel = PA_LEGR0,     .pos = {      0,  -8*256, -24*256 } },
		[PA_FOOTL]     = { .rel = PA_LEGL1,     .pos = {      0,   4*256, -24*256 } },
		[PA_FOOTR]     = { .rel = PA_LEGR1,     .pos = {      0,  -4*256, -24*256 } },
		[PA_ARML0]     = { .rel = PA_SHOULDERS, .pos = {  3*256,  20*256,  -8*256 } },
		[PA_ARML1]     = { .rel = PA_ARML0,     .pos = {  3*256,  14*256, -12*256 } },
		[PA_ARMR0]     = { .rel = PA_SHOULDERS, .pos = {  7*256, -20*256,  -8*256 } },
		[PA_ARMR1]     = { .rel = PA_ARMR0,     .pos = {  7*256, -14*256, -12*256 } },
		[PA_SHADOW]    = { .rel = 0,            .pos = {      0,       0,    -256 } },
	},
	
	//Run cycle - 4 frames, alternating contact/passing poses
	[AN_RUN0] = //Left leg up, right leg down, passing pose
	{
		[PA_HEAD]      = { .rel = PA_SHOULDERS, .pos = {      0,       0,  12*256 } },
		[PA_SHOULDERS] = { .rel = PA_TORSO,     .pos = {      0,       0,  24*256 } },
		[PA_TORSO]     = { .rel = 0,            .pos = {      0,       0,  80*256 } },
		[PA_HIPS]      = { .rel = PA_TORSO,     .pos = {      0,       0, -24*256 } },
		[PA_LEGL0]     = { .rel = PA_HIPS,      .pos = {  4*256,   8*256,  -4*256 } },
		[PA_LEGL1]     = { .rel = PA_LEGL0,     .pos = { 18*256,   8*256, -12*256 } },
		[PA_LEGR0]     = { .rel = PA_HIPS,      .pos = {      0,  -8*256,  -8*256 } },
		[PA_LEGR1]     = { .rel = PA_LEGR0,     .pos = {      0,  -8*256, -24*256 } },
		[PA_FOOTL]     = { .rel = PA_LEGL1,     .pos = {-16*256,   4*256, -14*256 } },
		[PA_FOOTR]     = { .rel = PA_LEGR1,     .pos = {      0,  -4*256, -24*256 } },
		[PA_ARML0]     = { .rel = PA_SHOULDERS, .pos = {  3*256,  20*256,  -8*256 } },
		[PA_ARML1]     = { .rel = PA_ARML0,     .pos = {  3*256,  14*256, -12*256 } },
		[PA_ARMR0]     = { .rel = PA_SHOULDERS, .pos = {  7*256, -20*256,  -8*256 } },
		[PA_ARMR1]     = { .rel = PA_ARMR0,     .pos = {  7*256, -14*256, -12*256 } },
		[PA_SHADOW]    = { .rel = 0,            .pos = {      0,       0,    -256 } },
	},
	[AN_RUN1] = //Left leg forward, right leg back, contact pose
	{
		[PA_HEAD]      = { .rel = PA_SHOULDERS, .pos = {      0,       0,  12*256 } },
		[PA_SHOULDERS] = { .rel = PA_TORSO,     .pos = {      0,       0,  24*256 } },
		[PA_TORSO]     = { .rel = 0,            .pos = {      0,       0,  72*256 } },
		[PA_HIPS]      = { .rel = PA_TORSO,     .pos = {      0,       0, -24*256 } },
		[PA_LEGL0]     = { .rel = PA_HIPS,      .pos = {  2*256,   8*256,  -6*256 } },
		[PA_LEGL1]     = { .rel = PA_LEGL0,     .pos = { 16*256,   8*256, -14*256 } },
		[PA_LEGR0]     = { .rel = PA_HIPS,      .pos = { -2*256,  -8*256,  -6*256 } },
		[PA_LEGR1]     = { .rel = PA_LEGR0,     .pos = {-16*256,  -8*256, -14*256 } },
		[PA_FOOTL]     = { .rel = PA_LEGL1,     .pos = {  6*256,   4*256, -20*256 } },
		[PA_FOOTR]     = { .rel = PA_LEGR1,     .pos = { -6*256,  -4*256, -20*256 } },
		[PA_ARML0]     = { .rel = PA_SHOULDERS, .pos = {  3*256,  20*256,  -8*256 } },
		[PA_ARML1]     = { .rel = PA_ARML0,     .pos = {  3*256,  14*256, -12*256 } },
		[PA_ARMR0]     = { .rel = PA_SHOULDERS, .pos = {  7*256, -20*256,  -8*256 } },
		[PA_ARMR1]     = { .rel = PA_ARMR0,     .pos = {  7*256, -14*256, -12*256 } },
		[PA_SHADOW]    = { .rel = 0,            .pos = {      0,       0,    -256 } },
	},
	[AN_RUN2] = //Left leg down, right leg up, passing pose
	{
		[PA_HEAD]      = { .rel = PA_SHOULDERS, .pos = {      0,       0,  12*256 } },
		[PA_SHOULDERS] = { .rel = PA_TORSO,     .pos = {      0,       0,  24*256 } },
		[PA_TORSO]     = { .rel = 0,            .pos = {      0,       0,  80*256 } },
		[PA_HIPS]      = { .rel = PA_TORSO,     .pos = {      0,       0, -24*256 } },
		[PA_LEGR0]     = { .rel = PA_HIPS,      .pos = {  4*256,  -8*256,  -4*256 } },
		[PA_LEGR1]     = { .rel = PA_LEGR0,     .pos = { 18*256,  -8*256, -12*256 } },
		[PA_LEGL0]     = { .rel = PA_HIPS,      .pos = {      0,   8*256,  -8*256 } },
		[PA_LEGL1]     = { .rel = PA_LEGL0,     .pos = {      0,   8*256, -24*256 } },
		[PA_FOOTR]     = { .rel = PA_LEGR1,     .pos = {-16*256,  -4*256, -14*256 } },
		[PA_FOOTL]     = { .rel = PA_LEGL1,     .pos = {      0,   4*256, -24*256 } },
		[PA_ARML0]     = { .rel = PA_SHOULDERS, .pos = {  3*256,  20*256,  -8*256 } },
		[PA_ARML1]     = { .rel = PA_ARML0,     .pos = {  3*256,  14*256, -12*256 } },
		[PA_ARMR0]     = { .rel = PA_SHOULDERS, .pos = {  7*256, -20*256,  -8*256 } },
		[PA_ARMR1]     = { .rel = PA_ARMR0,     .pos = {  7*256, -14*256, -12*256 } },
		[PA_SHADOW]    = { .rel = 0,            .pos = {      0,       0,    -256 } },
	},	
	[AN_RUN3] = //Left leg back, right leg forward, contact pose
	{
		[PA_HEAD]      = { .rel = PA_SHOULDERS, .pos = {      0,       0,  12*256 } },
		[PA_SHOULDERS] = { .rel = PA_TORSO,     .pos = {      0,       0,  24*256 } },
		[PA_TORSO]     = { .rel = 0,            .pos = {      0,       0,  72*256 } },
		[PA_HIPS]      = { .rel = PA_TORSO,     .pos = {      0,       0, -24*256 } },
		[PA_LEGL0]     = { .rel = PA_HIPS,      .pos = { -2*256,   8*256,  -6*256 } },
		[PA_LEGL1]     = { .rel = PA_LEGL0,     .pos = {-16*256,   8*256, -14*256 } },
		[PA_LEGR0]     = { .rel = PA_HIPS,      .pos = {  2*256,  -8*256,  -6*256 } },
		[PA_LEGR1]     = { .rel = PA_LEGR0,     .pos = { 16*256,  -8*256, -14*256 } },
		[PA_FOOTL]     = { .rel = PA_LEGL1,     .pos = { -6*256,   4*256, -20*256 } },
		[PA_FOOTR]     = { .rel = PA_LEGR1,     .pos = {  6*256,  -4*256, -20*256 } },
		[PA_ARML0]     = { .rel = PA_SHOULDERS, .pos = {  3*256,  20*256,  -8*256 } },
		[PA_ARML1]     = { .rel = PA_ARML0,     .pos = {  3*256,  14*256, -12*256 } },
		[PA_ARMR0]     = { .rel = PA_SHOULDERS, .pos = {  7*256, -20*256,  -8*256 } },
		[PA_ARMR1]     = { .rel = PA_ARMR0,     .pos = {  7*256, -14*256, -12*256 } },
		[PA_SHADOW]    = { .rel = 0,            .pos = {      0,       0,    -256 } },
	},
	
};

static bool chanim_flattened = false;

void chanim(chanim_idx_t anim, int facing, int vx, int vy, int vz)
{
	if(!chanim_flattened)
	{			
		//Flatten animation data so everything is just relative to the model root
		for(int aa = 0; aa < AN_MAX; aa++)
		{
			for(int pass = 0; pass < PA_MAX; pass++)
			{
				for(int pp = 0; pp < PA_MAX; pp++)
				{
					int rel = chanim_table[aa][pp].rel;
					chanim_table[aa][pp].pos[0] += chanim_table[aa][rel].pos[0];
					chanim_table[aa][pp].pos[1] += chanim_table[aa][rel].pos[1];
					chanim_table[aa][pp].pos[2] += chanim_table[aa][rel].pos[2];
					chanim_table[aa][pp].rel = chanim_table[aa][rel].rel;
				}
			}
		}
		chanim_flattened = true;
	}
	
	for(int pp = 0; pp < PA_MAX; pp++)
	{
		int64_t partpos[3] = {0};
		
		//Origin of model
		partpos[0] += (int64_t)vx * (int64_t)256;
		partpos[1] += (int64_t)vy * (int64_t)256;
		partpos[2] += (int64_t)vz * (int64_t)256;
		
		//Local X component
		partpos[0] += (int64_t)chanim_table[anim][pp].pos[0] * (int64_t)trigfunc_cos8(facing);
		partpos[1] += (int64_t)chanim_table[anim][pp].pos[0] * (int64_t)trigfunc_sin8(facing);
		
		//Local Y component
		partpos[0] -= (int64_t)chanim_table[anim][pp].pos[1] * (int64_t)trigfunc_sin8(facing);
		partpos[1] += (int64_t)chanim_table[anim][pp].pos[1] * (int64_t)trigfunc_cos8(facing);
		
		//Local Z component
		partpos[2] += (int64_t)chanim_table[anim][pp].pos[2] * (int64_t)256;
		
		//Reduce back down to 8 bits of fraction
		partpos[0] /= 256;
		partpos[1] /= 256;
		partpos[2] /= 256;
		
		proj_card(chanim_part_table[pp].imf, partpos[0], partpos[1], partpos[2], chanim_part_table[pp].vh);
	}
}
