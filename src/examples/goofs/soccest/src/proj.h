//proj.h
//Projection/3D drawing functions
//Bryan E. Topp <betopp@betopp.com> 2025
#ifndef PROJ_H
#define PROJ_H

#include "images.h"
#include <stdint.h>

//Sets camera center for projection drawing.
//cx8/cy8/cz8 are 24.8 fixed-point values for the point at camera focus.
//fov is the radius visible at camera focus.
void proj_cam(int32_t cx8, int32_t cy8, int32_t cz8, int32_t fov);

//Enqueues drawing a card at virtual coordinates.
//imf is the index of the image to draw.
//vx8/vy8/vz8 are 24.8 fixed-point virtual coordinates, relative to the camera center (proj_cam).
//vh8 is the 24.8 fixed-point virtual height of the card - width is scaled appropriately.
void proj_card(images_file_t imf, int32_t vx8, int32_t vy8, int32_t vz8, int32_t vh8);

//Sorts and draws projected images enqueued.
void proj_draw(void);

#endif //PROJ_H
