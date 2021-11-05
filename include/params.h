#include <semaphore.h>

//VT100 escape sequences
#define ANSI_CLEAR "\x1b[2J"
#define ANSI_LINE_CLEAR "\x1b[2K"
#define ANSI_HOME "\x1b[H"

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
//out-of-bounds array index...
//...or just snap the points to the edges when needed
#define ZPRIMEX 26
#define ZPRIMEY 10

//lower values correspond to more points
#define THETA_INC 0.07
#define PHI_INC 0.02

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
    int xp;
    int yp;
    float z_inv;
    float lum;
} point_t;


//contains all point data necessary for one frame
typedef struct
{
    point_t *points;
    size_t n;

    sem_t full;
    sem_t empty;
} image_out;

//contains the last key that was pressed
typedef struct
{
    int buf;

    sem_t full;
    sem_t empty;
} user_in;

typedef struct
{
    //affects rotation on x
	float A;
	//affects rotation on z
	float B;

	//translate the object
	int offsetx;
	int offsety;

	//location of light source
	float light_src[1][3];

    image_out img;
} render_args;