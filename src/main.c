#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>
#include "matrix.h"
#include "params.h"

//holds the x and y projection along with
//z-buffer and luminance data for a single point
typedef struct
{
	int xp;
	int yp;
	float z_inv;
	float lum;
} point_t;

//affects rotation on x
static float A = 0;
//affects rotation on z
static float B = 0;

//translate the object
static int offsetx = 0;
static int offsety = 0;

//required to translate all points to a region
//that is in front of the viewer
static float dist;

//toggle for representing luminosity
//as defined by the character sequence
static char lighting = 1;

//location of light source
static float light_src[1][3];

//contains all point data necessary for one frame
static struct
{
	point_t *points;

	sem_t full;
	sem_t empty;
} image_out;

//contains the last key that was pressed
static struct
{
	char buf;

	sem_t full;
	sem_t empty;
} user_in;


void *ctrl(void *ptr)
{
	while (1)
	{
		sem_wait(&user_in.empty);
		int r = read(STDIN_FILENO, &user_in.buf, 1);
		sem_post(&user_in.full);
	}
}

void *render(void *ptr)
{
	while (1)
	{
		sem_wait(&image_out.empty);

		size_t index = 0;
		for (float theta = 0; theta < 2 * M_PI; theta += THETA_INC)
		{
			for (float phi = 0; phi < 2 * M_PI; phi += PHI_INC)
			{
				//rotate a point of the torus
				float point[1][3] = {{R2 + R1 * cos(theta), R1 * sin(theta), 0}};
				float pointbuf[3][1][3];
				rotate_mat(1, phi, 1, point, pointbuf[0]);
				rotate_mat(1, A, 0, pointbuf[0], pointbuf[1]);
				rotate_mat(1, B, 2, pointbuf[1], pointbuf[2]);
				//ensure the point is in front of the viewer
				pointbuf[2][0][2] += dist;

				point_t temp;
				temp.z_inv = 1 / pointbuf[2][0][2];
				temp.xp = (int)(COL / 2 + pointbuf[2][0][0] * ZPRIMEX * temp.z_inv) + offsetx;
				temp.yp = (int)(ROW / 2 - pointbuf[2][0][1] * ZPRIMEY * temp.z_inv) + offsety;
				
				//accommodate if projection is out-of-bounds
				char invalid_pos =
					temp.xp < 0
					|| temp.xp > COL - 1
					|| temp.yp < 0
					|| temp.yp > ROW - 1;
				if (invalid_pos)
				{
					temp.xp = temp.yp = temp.z_inv = 0;
				}

				if (lighting)
				{
					//perform the same rotations with unit circle
					//to find surface normal
					float norm[1][3] = {{cos(theta), sin(theta), 0}};
					float normbuf[3][1][3];
					rotate_mat(1, phi, 1, norm, normbuf[0]);
					rotate_mat(1, A, 0, normbuf[0], normbuf[1]);
					rotate_mat(1, B, 2, normbuf[1], normbuf[2]);
					//dot product of normal vector and light source
					//1 parallel, -1 anti-parallel, 0 perpendicular
					temp.lum = dot_mat(3, normbuf[2], light_src);
				}

				//capture the results for this point (for this frame) in
				//the synchronized array
				image_out.points[index] = temp;
				++index;
			}
		}

		sem_post(&image_out.full);
	}
}

int main(int argc, char **argv)
{
	//clear the screen
	printf("\x1b[2J");

	//change terminal to noncanonical mode
	struct termios t = {0};
	tcgetattr(STDOUT_FILENO, &t);
    t.c_lflag &= ~ICANON;
    t.c_lflag &= ~ECHO;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDOUT_FILENO, TCSANOW, &t);

	//let the object rotate automatically
	char auto_mode = 1;

	//avoid issues with precision on different platforms
	size_t count = 0;
	for (float i = 0; i < 2 * M_PI; i += THETA_INC)
		for (float j = 0; j < 2 * M_PI; j += PHI_INC)
			count++;
	image_out.points = (point_t *)malloc(sizeof(point_t) * count);

	//vector must be normalized
	light_src[0][0] = 0;
	light_src[0][1] = 1 / sqrt(2);
	light_src[0][2] = -1 / sqrt(2);

	//final z coordinate for each point
	//must be >0 so resulting object is
	//entirely in front of the viewer
	dist = R1 + R2 + 1;

	//toggle lighting based on args
	lighting = argc > 1 && !strcmp(argv[1], "-nl") ? 0 : 1;

	//setup synchronization
	sem_init(&image_out.empty, 0, 1);
	sem_init(&image_out.full, 0, 0);
	sem_init(&user_in.empty, 0, 1);
	sem_init(&user_in.full, 0, 0);
	//start render and input thread
	pthread_t t_render, t_input;
	pthread_create(&t_render, NULL, render, NULL);
	pthread_create(&t_input, NULL, ctrl, NULL);

	char frame[ROW][COL];
	float zbuffer[ROW][COL];
	while (1)
	{
		memset(frame, ' ', sizeof(frame));
		//z-buffer values of 0 correspond to infinite distance
		memset(zbuffer, 0, sizeof(zbuffer));

		sem_wait(&image_out.full);

		//process the results of the render thread by choosing
		//which points will actually be shown on the projection
		//using a z-buffer
		for (size_t i = 0; i < count; ++i)
		{
			int xp = image_out.points[i].xp;
			int yp = image_out.points[i].yp;
			float z_inv = image_out.points[i].z_inv;
			float lum = image_out.points[i].lum;

			if (z_inv > zbuffer[yp][xp])
			{
				zbuffer[yp][xp] = z_inv;
				frame[yp][xp] = '.';
				//luminance values of less than zero imply that the
				//angle between the vectors is more than 90 degrees
				//just leave the character as a period in this case
				if (lighting && lum > 0)
				{
					//scale-up the luminance and pick a character
					size_t mult = sizeof(LIGHTSYM) / sizeof(LIGHTSYM[0]) - 1;
					frame[yp][xp] = LIGHTSYM[(int)(lum * mult)];
				}
			}
		}

		if (auto_mode)
		{
			//increment so the next frame changes angle
			A += A_INC;
			B += B_INC;
		}

		//update render params for next frame based on input
		if (sem_trywait(&user_in.full) == 0)
		{
			switch(user_in.buf)
			{
				case 32:
					auto_mode = !auto_mode;
					break;
				case 117:
					offsety -= 1;
					break;
				case 106:
					offsety += 1;
					break;
				case 104:
					offsetx -=1;
					break;
				case 107:
					offsetx += 1;
					break;
			}

			sem_post(&user_in.empty);
		}

		sem_post(&image_out.empty);

		//move cursor to home before printing frame
		printf("\x1b[H");

		for (size_t i = 0; i < ROW; ++i)
		{
			for (size_t j = 0; j < COL; ++j)
			{
				putchar(frame[i][j]);
				fflush(0);
			}
			putchar('\n');
		}

		//we can print whatever info we want at this point
		printf("\x1b[2K");
		printf("X_ROT:%f\tZ_ROT:%f\n", fmod(A, M_PI), fmod(B, M_PI));
		fflush(0);

		usleep(SLEEPTIME);
	}

	pthread_join(t_render, NULL);
	pthread_join(t_input, NULL);
}
