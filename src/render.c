#include <time.h>
#include <math.h>
#include "matrix.h"
#include "params.h"

void render_frame(render_args *r_args)
{
    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    size_t index = 0;
    for (size_t i = 0; i < r_args->outer; ++i)
    {
        float theta = (float) i * THETA_INC;

        for (size_t j = 0; j < r_args->inner; ++j)
        {
            float phi = (float) j * PHI_INC;

            //rotate a point of the torus
            float point[1][3] = {{R2 + R1 * cos(theta), R1 * sin(theta), 0}};
            float pointbuf[3][1][3];
            rotate_mat(1, phi, aY, point, pointbuf[0]);
            rotate_mat(1, r_args->A, aX, pointbuf[0], pointbuf[1]);
            rotate_mat(1, r_args->B, aZ, pointbuf[1], pointbuf[2]);
            //ensure the point is in front of the viewer
            pointbuf[2][0][2] += DIST;

            point_t temp;
            temp.z_inv = 1 / pointbuf[2][0][2];
            temp.xp = (int)(COL / 2 + pointbuf[2][0][0] * ZPRIMEX * temp.z_inv) + r_args->offsetx;
            temp.yp = (int)(ROW / 2 - pointbuf[2][0][1] * ZPRIMEY * temp.z_inv) + r_args->offsety;

            //handle case when projection is out-of-bounds
            unsigned char invalid_pos = temp.xp < 0
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
            rotate_mat(1, phi, aY, norm, normbuf[0]);
            rotate_mat(1, r_args->A, aX, normbuf[0], normbuf[1]);
            rotate_mat(1, r_args->B, aZ, normbuf[1], normbuf[2]);
            //dot product of surface normal and light source
            //1 parallel, -1 anti-parallel, 0 perpendicular
            temp.lum = dot_mat(3, normbuf[2], r_args->light_src);

            //capture the results for this point (for this frame)
            r_args->points[index++] = temp;
        }
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);

    //find cpu time
    float diff_s = end.tv_sec - start.tv_sec;
    float diff_ns;

    if (diff_s > 0)
        diff_ns = (1e9 - start.tv_nsec) + ((diff_s - 1) * 1e9) + (end.tv_nsec);
    else
        diff_ns = end.tv_nsec - start.tv_nsec;

    r_args->frame_time = diff_ns / 1e6;
}
