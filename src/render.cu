#include "params.h"

enum rotationAxis {aX = 0, aY = 1, aZ = 2};

__device__ void d_mult_mat(
        size_t      m1,
        size_t      n1,
        size_t      m2,
        size_t      n2,
        const float *a,
        const float *b,
        float       *buf)
{
    for (size_t i = 0; i < m1; ++i)
        for (size_t j = 0; j < n2; ++j)
        {
            float sum = 0;
            for (size_t k = 0; k < m2; ++k)
                sum += a[i * n1 + k] * b[k * n2 + j];
            buf[i * n2 + j] = sum;
        }
}

__device__ void d_rotate_mat(
        size_t            m1,
        float             phi,
        enum rotationAxis axis,
        const float       *a,
        float             *buf)
{
    switch (axis)
    {
        case aX:
            {
                float R_x[9] =
                {
                    1, 0,        0,
                    0, cos(phi), -1 * sin(phi),
                    0, sin(phi), cos(phi)
                };
                d_mult_mat(m1, 3, 3, 3, a, R_x, buf);
                break;
            }
        case aY:
            {
                float R_y[9] =
                {
                    cos(phi),      0, sin(phi),
                    0,             1, 0,
                    -1 * sin(phi), 0, cos(phi)
                };
                d_mult_mat(m1, 3, 3, 3, a, R_y, buf);
                break;
            }
        case aZ:
            {
                float R_z[9] =
                {
                    cos(phi), -1 * sin(phi), 0,
                    sin(phi), cos(phi),      0,
                    0,        0,             1
                };
                d_mult_mat(m1, 3, 3, 3, a, R_z, buf);
                break;
            }
    }
}

__device__ void d_trans_mat(
        size_t      m,
        size_t      n,
        const float *a,
        float       *buf)
{
    for (size_t i = 0; i < m * n; ++i)
    {
        size_t x = i / m;
        size_t y = i % m;
        buf[i] = a[n * y + x];
    }
}

__global__ void render_frame(
        float   A,
        float   B,
        int     offsetx,
        int     offsety,
        float   lightx,
        float   lighty,
        float   lightz,
        size_t  outer,
        size_t  inner,
        point_t *points)
{
    size_t row = blockIdx.y * blockDim.y + threadIdx.y;
    size_t col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < outer && col < inner)
    {
        size_t gindex = row * inner + col;
        float  theta  = (float) row * THETA_INC;
        float  phi    = (float) col * PHI_INC;

        //rotate a point of the torus
        float point[3] = {R2 + R1 * cos(theta), R1 * sin(theta), 0};
        float pointbuf[3][3];
        d_rotate_mat(1, phi, aY, point, pointbuf[0]);
        d_rotate_mat(1, A, aX, pointbuf[0], pointbuf[1]);
        d_rotate_mat(1, B, aZ, pointbuf[1], pointbuf[2]);
        //ensure the point is in front of the viewer
        pointbuf[2][2] += DIST;

        point_t temp;
        temp.z_inv = 1 / pointbuf[2][2];
        temp.xp = (int)(COL / 2 + pointbuf[2][0] * ZPRIMEX * temp.z_inv) + offsetx;
        temp.yp = (int)(ROW / 2 - pointbuf[2][1] * ZPRIMEY * temp.z_inv) + offsety;

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
        float norm[3] = {cos(theta), sin(theta), 0};
        float normbuf[3][3];
        d_rotate_mat(1, phi, aY, norm, normbuf[0]);
        d_rotate_mat(1, A, aX, normbuf[0], normbuf[1]);
        d_rotate_mat(1, B, aZ, normbuf[1], normbuf[2]);
        //dot product of surface normal and light source
        //1 parallel, -1 anti-parallel, 0 perpendicular
        float light_src[3] = {lightx, lighty, lightz}, light_src_transpose[3], result[1];
        d_trans_mat(1, 3, light_src, light_src_transpose);
        d_mult_mat(1, 3, 3, 1, normbuf[2], light_src_transpose, result);
        temp.lum = result[0];

        //capture the results for this point (for this frame)
        points[gindex] = temp;
    }
}

extern "C" void cuda_render_frame(render_args *r_args)
{
    point_t *d_points;
    size_t points_arr_size = sizeof(point_t) * r_args->outer * r_args->inner;
    cudaMalloc(&d_points, points_arr_size);

    dim3 dimBlock(32, 4);
    dim3 dimGrid((int) ceil(r_args->inner * 1.0f / dimBlock.x),
            (int) ceil(r_args->outer * 1.0f / dimBlock.y));
    cudaEvent_t start, stop;

    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);
    render_frame<<<dimGrid,dimBlock>>>(
            r_args->A,
            r_args->B,
            r_args->offsetx,
            r_args->offsety,
            r_args->light_src[0][0],
            r_args->light_src[0][1],
            r_args->light_src[0][2],
            r_args->outer,
            r_args->inner,
            d_points);
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&r_args->frame_time, start, stop);

    cudaMemcpy(r_args->points, d_points, points_arr_size, cudaMemcpyDeviceToHost);
    cudaFree(d_points);
}

extern "C" void cuda_device_reset()
{
    cudaDeviceReset();
}
