//chanim.h
//Functions for animating characters
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef CHANIM_H
#define CHANIM_H

//Frames of animation that character can be in
typedef enum chanim_idx_e
{
	AN_NONE = 0,
	AN_STAND,
	AN_RUN0,
	AN_RUN1,
	AN_RUN2,
	AN_RUN3,
	AN_MAX
} chanim_idx_t;

//Enqueues drawing character at screen-relative 3d coordinates.
//Facing is a 0.16 angle where 65536 is a whole circle
//Rel_vx/vy/vz are 24.8 fixed-point centimeters relative to screen center
//Graphics are enqueued with proj_card - call proj_draw to draw to screen
void chanim(chanim_idx_t anim, int facing16, int rel_vx8, int rel_vy8, int rel_vz8);

#endif //CHANIM_H
