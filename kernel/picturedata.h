#ifndef __PICTUREDATA_H
#define __PICTUREDATA_H

#include "types.h"

#define PICDATA_WIDTH 640
#define PICDATA_HEIGHT 480
#define PICDATA_BPP 4 // bytes per pixel (usually it means bits per pixel)
extern uint8_t picturedata[PICDATA_WIDTH * PICDATA_HEIGHT * PICDATA_BPP];

#endif // __PICTUREDATA_H