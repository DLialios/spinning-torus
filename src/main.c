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
#include "render.h"



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
		int r = read(STDIN_FILENO, &input->buf, 1);
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

	size_t count = 0;
	for (float i = 0; i < 2 * M_PI; i += THETA_INC)
		for (float j = 0; j < 2 * M_PI; j += PHI_INC)
			count++;
	r->img.n = count;
	r->img.points = (point_t*) malloc(sizeof(point_t) * count);	
	
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
		for (size_t i = 0; i < r->img.n; ++i)
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
				case 'u':
					r->offsety -= 1;
					break;
				case 'j':
					r->offsety += 1;
					break;
				case 'h':
					r->offsetx -=1;
					break;
				case 'k':
					r->offsetx += 1;
					break;
			}

			if (exit_loop)
				break;	

			sem_post(&u->empty);
		}

		sem_post(&r->img.empty);

		print_frame(frame);
		print_info(r->A, r->B);

		usleep(SLEEPTIME);
	}
}

void signal_handler(int sig)
{
	switch (sig)
	{
		case SIGINT:
		break;
	}
}

int main(int argc, char **argv)
{

	sigset_t mask;
	sigfillset(&mask);
	pthread_sigmask(SIG_BLOCK, &mask, NULL);


	// struct sigaction sa;
	// sigset_t mask;
	// sigemptyset(&mask);
	// sa.sa_handler = &signal_handler;
	// sa.sa_mask = mask;



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
	free(r.img.points);
	free(prev_terminal_state);
}

