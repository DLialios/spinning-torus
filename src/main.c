#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>
#include <signal.h>
#include "render.h"
#include "params.h"
#include "matrix.h"
#include "rendercu.h"

void init_r(render_args *r)
{
    r->rKind            = INIT_RKIND;
    r->A                = 0;
    r->B                = 0;
    r->offsetx          = 0;
    r->offsety          = 0;
    r->light_src[0][0]  = 0;
    r->light_src[0][1]  = 1 / sqrt(2);
    r->light_src[0][2]  = -1 / sqrt(2);

    size_t outer = 0, inner = 0;
    for (float i = 0; i < 2 * M_PI; i += THETA_INC)
        ++outer;
    for (float j = 0; j < 2 * M_PI; j += PHI_INC)
        ++inner;

    r->outer  = outer;
    r->inner  = inner;
    r->points = (point_t*) malloc(sizeof(point_t) * outer * inner);

    sem_init(&r->empty, 0, 1);
    sem_init(&r->full, 0, 0);
}

void init_u(user_in *u)
{
    u->buf = 0;
    sem_init(&u->empty, 0, 1);
    sem_init(&u->full, 0, 0);
}

void * render(void *ptr)
{

#ifdef CUDA_R
    static enum renderer prev_renderer = INIT_RKIND;
#endif

    render_args *in = (render_args*) ptr;

    while (1)
    {
        sem_wait(&in->empty);

#ifdef CUDA_R
        if (in->rKind == software && prev_renderer == cuda)
            cuda_device_reset();

        if (in->rKind == software)
            render_frame(in);
        else
            cuda_render_frame(in);

        prev_renderer = in->rKind;
#endif

#ifndef CUDA_R
        render_frame(in);
#endif

        sem_post(&in->full);
    }
}

void * ctrl(void *ptr)
{
    user_in *in = (user_in*) ptr;

    while (1)
    {
        sem_wait(&in->empty);

        if (read(STDIN_FILENO, &in->buf, 1))
            ;

        sem_post(&in->full);
    }
}

struct termios* set_noncanonical()
{
    struct termios *prev_state = (struct termios*) malloc(sizeof(struct termios));
    struct termios t;
    tcgetattr(STDOUT_FILENO, prev_state);
    tcgetattr(STDOUT_FILENO, &t);

    t.c_lflag     &= ~ICANON;
    t.c_lflag     &= ~ECHO;
    t.c_cc[VMIN]  =  1;
    t.c_cc[VTIME] =  0;

    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &t);
    return prev_state;
}

void draw_frame_loop(render_args *r, user_in *u)
{
    char          frame[ROW][COL];
    float         zbuf[ROW][COL];
    unsigned char auto_mode = 1,
                  exit_loop = 0;

    while (1)
    {
        memset(frame, ' ', sizeof(frame));
        memset(zbuf, 0, sizeof(zbuf)); //z-buffer = 0 -> infinite distance

        sem_wait(&r->full);

        //choose which points to project
        for (size_t i = 0; i < r->outer * r->inner; ++i)
        {
            int   xp    = r->points[i].xp;
            int   yp    = r->points[i].yp;
            float z_inv = r->points[i].z_inv;
            float lum   = r->points[i].lum;

            if (z_inv > zbuf[yp][xp])
            {
                zbuf[yp][xp]  = z_inv;
                frame[yp][xp] = '.';

                if (lum > 0)
                {
                    size_t mult   = sizeof(LIGHTSYM) / sizeof(LIGHTSYM[0]) - 1;
                    frame[yp][xp] = LIGHTSYM[(int) (lum * mult)];
                }
            }
        }

        if (auto_mode)
        {
            //change angle for next frame
            r->A += A_INC;
            r->B += B_INC;
        }

        //use keyboard input
        if (sem_trywait(&u->full) == 0)
        {
            switch (u->buf)
            {
                case ASCII_ESCAPE:
                    exit_loop = 1;
                    break;
                case ASCII_SPACE:
                    auto_mode = !auto_mode;
                    break;
                case 'k':
                    r->offsety -= 1;
                    break;
                case 'j':
                    r->offsety += 1;
                    break;
                case 'h':
                    r->offsetx -=1;
                    break;
                case 'l':
                    r->offsetx += 1;
                    break;
#ifdef CUDA_R
                case 'g':
                    r->rKind = r->rKind == software ? cuda : software;
                    break;
#endif
            }

            if (exit_loop)
                break;

            sem_post(&u->empty);
        }

        sem_post(&r->empty);

        //print frame with info
        printf(ANSI_HOME);
        for (size_t i = 0; i < ROW; ++i)
        {
            for (size_t j = 0; j < COL; ++j)
                putchar(frame[i][j]);
            putchar('\n');
        }
        printf(ANSI_SET_BOLD);
        printf(ANSI_LINE_CLEAR "X_ROT\t:%.2frad\n", fmod(r->A, M_PI));
        printf(ANSI_LINE_CLEAR "Z_ROT\t:%.2frad\n", fmod(r->B, M_PI));
        printf(ANSI_LINE_CLEAR "%s\t:%.3fms\n", r->rKind == software ? "SW" : "CU", r->frame_time);
        printf(ANSI_RESET_SGR);

        usleep(SLEEPTIME);
    }
}

int main(int argc, char **argv)
{
    sigset_t       mask;
    struct termios *prev_terminal_state;
    render_args    r;
    user_in        u;
    pthread_t      t_render,
                   t_input;

    sigfillset(&mask);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    printf(ANSI_CLEAR ANSI_HOME ANSI_HIDE_CURSOR);
    prev_terminal_state = set_noncanonical();

    init_r(&r);
    init_u(&u);
    pthread_create(&t_render, NULL, render, (void*) &r);
    pthread_create(&t_input, NULL, ctrl, (void*) &u);

    draw_frame_loop(&r, &u);

    printf(ANSI_CLEAR ANSI_HOME ANSI_SHOW_CURSOR);
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, prev_terminal_state);

    free(prev_terminal_state);
    free(r.points);
}

