#include <stdlib.h>
#include <math.h>
#include "matrix.h"
#include "params.h"

void *render(void *ptr)
{
	render_args *input = (render_args*) ptr;

	while (1)
	{
		sem_wait(&input->img.empty);

		size_t index = 0;
		for (float theta = 0; theta < 2 * M_PI; theta += THETA_INC)
		{
			for (float phi = 0; phi < 2 * M_PI; phi += PHI_INC)
			{
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

		sem_post(&input->img.full);
	}
}