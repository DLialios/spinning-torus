#ifndef RENDER_H
#define RENDER_H

#include "params.h"

extern void cuda_render_frame(render_args *r_args);
extern void cuda_device_reset();

#endif