#ifndef PARAMS_H
#define PARAMS_H

#include <semaphore.h>

#define ASCII_ESCAPE 27
#define ASCII_SPACE  32

//escape sequences
#define ANSI_CLEAR       "\x1b[2J"
#define ANSI_LINE_CLEAR  "\x1b[2K"
#define ANSI_HOME        "\x1b[H"
#define ANSI_SET_BOLD    "\x1b[1m"
#define ANSI_RESET_SGR   "\x1b[0m"
#define ANSI_HIDE_CURSOR "\x1b[?25l"
#define ANSI_SHOW_CURSOR "\x1b[?25h"

//output size
#define ROW 24
#define COL 60

//radius of circle
#define R1 1
//radius of torus
#define R2 2

//final z coordinate for each point
//must be >0 so resulting object is
//entirely in front of the viewer
#define DIST R1 + R2 + 1

//there's an optimization problem for computing these vals
//such that the resultant projection does not produce an
//out-of-bounds array index
#define ZPRIMEX 26
#define ZPRIMEY 10

//lower values correspond to more points
#define THETA_INC 0.07
#define PHI_INC   0.02

//x-axis rotation speed
#define A_INC 0.05
//z-axis rotation speed
#define B_INC 0.03

//~33ms per frame for 30fps
#define SLEEPTIME 33333

//symbols for luminance values
#define LIGHTSYM ",:;-~=*#&$@"

//holds the x and y projection along with
//z-buffer and luminance data for a single point
typedef struct
{
        int   xp;
        int   yp;
        float z_inv;
        float lum;
} point_t;

//contains the last key that was pressed
typedef struct
{
        int buf;

        sem_t full;
        sem_t empty;
} user_in;

enum renderer {software = 1, cuda = 0};

typedef struct
{
        enum renderer rKind;

        float frame_time;

        //x-axis rotation
        float A;
        //z-axis rotation
        float B;

        //object translation
        int offsetx;
        int offsety;

        //location of light source
        float light_src[1][3];

        //avoid floating-point loop conditions
        size_t outer;
        size_t inner;

        point_t *points;

        sem_t full;
        sem_t empty;
} render_args;

#endif
