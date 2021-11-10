#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>
#include <signal.h>
#include "matrix.h"
#include "params.h"

extern void cuda_render_frame(render_args *input);
extern void cuda_device_reset();

void print_frame(char (*frame)[COL])
{
	printf(ANSI_HOME);

	for (size_t i = 0; i < ROW; ++i)
	{
		for (size_t j = 0; j < COL; ++j)
		{
			putchar(frame[i][j]);
			fflush(0);
		}
		putchar('\n');
	}	
}

void print_info(float a, float b)
{
	printf(ANSI_LINE_CLEAR);
	printf("X_ROT:%f\tZ_ROT:%f\n", fmod(a, M_PI), fmod(b, M_PI));

	fflush(0);
}


void *ctrl(void *ptr)
{
	user_in *input = (user_in*) ptr;
	while (1)
	{
		sem_wait(&input->empty);
		read(STDIN_FILENO, &input->buf, 1);
		sem_post(&input->full);
	}
}

struct termios* set_noncanonical()
{
	printf(ANSI_CLEAR);

	struct termios *prev_state = malloc(sizeof(struct termios));
	tcgetattr(STDOUT_FILENO, prev_state);

	struct termios t;
	tcgetattr(STDOUT_FILENO, &t);
    t.c_lflag &= ~ICANON;
    t.c_lflag &= ~ECHO;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDOUT_FILENO, TCSANOW, &t);

	return prev_state;
}

void set_canonical(struct termios *prev)
{
	printf(ANSI_CLEAR ANSI_HOME);
	tcsetattr(STDOUT_FILENO, TCSANOW, prev);
}

void init_r(render_args *r)
{
	r->A = 0;
	r->B = 0;
	r->offsetx = 0;
	r->offsety = 0;
	r->light_src[0][0] = 0;
	r->light_src[0][1] = 1 / sqrt(2);
	r->light_src[0][2] = -1 / sqrt(2);

	size_t outer = 0, inner = 0;
	for (float i = 0; i < 2 * M_PI; i += THETA_INC)
		++outer;
	for (float j = 0; j < 2 * M_PI; j += PHI_INC)
		++inner;
	r->img.outer = outer;
	r->img.inner = inner;
	r->img.points = (point_t*) malloc(sizeof(point_t) * outer * inner);	

	r->software_renderer = 1;
	
	sem_init(&r->img.empty, 0, 1);
	sem_init(&r->img.full, 0, 0);
}

void init_u(user_in *u)
{
	u->buf = 0;
	sem_init(&u->empty, 0, 1);
	sem_init(&u->full, 0, 0);
}

void draw_frame_loop(render_args *r, user_in *u)
{
	char frame[ROW][COL];
	float zbuffer[ROW][COL];
	unsigned char auto_mode = 1, exit_loop = 0;
	while (1)
	{
		memset(frame, ' ', sizeof(frame));
		//z-buffer values of 0 correspond to infinite distance
		memset(zbuffer, 0, sizeof(zbuffer));

		sem_wait(&r->img.full);

		//process the results of the render thread by choosing
		//which points will actually be shown on the projection
		//using a z-buffer
		for (size_t i = 0; i < r->img.outer * r->img.inner; ++i)
		{
			int xp = r->img.points[i].xp;
			int yp = r->img.points[i].yp;
			float z_inv = r->img.points[i].z_inv;
			float lum = r->img.points[i].lum;

			if (z_inv > zbuffer[yp][xp])
			{
				zbuffer[yp][xp] = z_inv;
				frame[yp][xp] = '.';
				//luminance values of less than zero imply that the
				//angle between the vectors is more than 90 degrees
				//just leave the character as a period in this case
				if (lum > 0)
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
			r->A += A_INC;
			r->B += B_INC;
		}

		//update render params for next frame based on input
		if (sem_trywait(&u->full) == 0)
		{
			switch(u->buf)
			{
				case 27:
					exit_loop = 1;
					break;
				case 32:
					auto_mode = !auto_mode;
					break;
				case 'w':
					r->offsety -= 1;
					break;
				case 's':
					r->offsety += 1;
					break;
				case 'a':
					r->offsetx -=1;
					break;
				case 'd':
					r->offsetx += 1;
					break;
				case 'g':
					r->software_renderer = 1 - r->software_renderer;
					break;
			}

			if (exit_loop)
				break;	

			sem_post(&u->empty);
		}

		sem_post(&r->img.empty);

		print_frame(frame);

	
		printf(ANSI_SET_BOLD);
		printf(ANSI_LINE_CLEAR);
		printf("X_ROT:%f\tZ_ROT:%f\n", fmod(r->A, M_PI), fmod(r->B, M_PI));
		printf(ANSI_LINE_CLEAR);
		printf("RENDERER: %s\n", r->software_renderer ? "SOFTWARE" : "CUDA");
		printf(ANSI_LINE_CLEAR);
		printf("FRAMETIME: %6.3f ms\n", r->last_frame_time);
		printf(ANSI_RESET_MODE);
		fflush(0);		

		usleep(SLEEPTIME);
	}
}


void render_frame(render_args *input)
{

	struct timespec start, end;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    size_t index = 0;
    for (size_t i = 0; i < input->img.outer; ++i)
    {
        float theta = (float) i * THETA_INC;

        for (size_t j = 0; j < input->img.inner; ++j)
        {
            float phi = (float) j * PHI_INC;

            //rotate a point of the torus
            float point[1][3] = {{R2 + R1 * cos(theta), R1 * sin(theta), 0}};
            float pointbuf[3][1][3];
            rotate_mat(1, phi, 1, point, pointbuf[0]);
            rotate_mat(1, input->A, 0, pointbuf[0], pointbuf[1]);
            rotate_mat(1, input->B, 2, pointbuf[1], pointbuf[2]);
            //ensure the point is in front of the viewer
            pointbuf[2][0][2] += DIST;

            point_t temp;
            temp.z_inv = 1 / pointbuf[2][0][2];
            temp.xp = (int)(COL / 2 + pointbuf[2][0][0] * ZPRIMEX * temp.z_inv) + input->offsetx;
            temp.yp = (int)(ROW / 2 - pointbuf[2][0][1] * ZPRIMEY * temp.z_inv) + input->offsety;
            
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

            //perform the same rotations with unit circle
            //to find surface normal
            float norm[1][3] = {{cos(theta), sin(theta), 0}};
            float normbuf[3][1][3];
            rotate_mat(1, phi, 1, norm, normbuf[0]);
            rotate_mat(1, input->A, 0, normbuf[0], normbuf[1]);
            rotate_mat(1, input->B, 2, normbuf[1], normbuf[2]);
            //dot product of normal vector and light source
            //1 parallel, -1 anti-parallel, 0 perpendicular
            temp.lum = dot_mat(3, normbuf[2], input->light_src);

            //capture the results for this point (for this frame) in
            //the synchronized array
            input->img.points[index] = temp;
            ++index;
        }
    }  	

	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);

	float diff_s = end.tv_sec - start.tv_sec;
	float diff_ns;

	if (diff_s > 0)
		diff_ns = (1e9 - start.tv_nsec) + ((diff_s - 1) * 1e9) + (end.tv_nsec);
	else
		diff_ns = end.tv_nsec - start.tv_nsec;

	input->last_frame_time = diff_ns / 1e6;
}


void *render(void *ptr)
{
	static unsigned char prev_renderer = 1;

	render_args *input = (render_args*) ptr;

	while (1)
	{
		sem_wait(&input->img.empty);

		if(input->software_renderer && !prev_renderer)
			cuda_device_reset();
		
		if (input->software_renderer)
			render_frame(input);
		else
			cuda_render_frame(input);

		prev_renderer = input->software_renderer;

		sem_post(&input->img.full);
	}
}


 
int main(int argc, char **argv)
{

	printf(ANSI_HIDE_CURSOR);

	sigset_t mask;
	sigfillset(&mask);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);

	render_args r;
	user_in u;
	pthread_t t_render, t_input;
	struct termios *prev_terminal_state;

	init_r(&r);
	init_u(&u);

	pthread_create(&t_render, NULL, render, (void*) &r);
	pthread_create(&t_input, NULL, ctrl, (void*) &u);

	prev_terminal_state = set_noncanonical();
	draw_frame_loop(&r, &u);
	
	set_canonical(prev_terminal_state);
	printf(ANSI_SHOW_CURSOR);
	free(r.img.points);
	free(prev_terminal_state);
}

