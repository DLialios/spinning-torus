

#include <stdio.h>

#include "params.h"


__device__ void d_mult_mat(size_t m1,
                           size_t n1,
                           size_t m2, 
                           size_t n2,
			               const float *a, 
                           const float *b, 
                           float *buf)
{
    for (int i = 0; i < m1; ++i)
        for (int j = 0; j < n2; ++j)
        {
            float sum = 0;
            for (int k = 0; k < m2; ++k)
                sum += a[i * n1 + k] * b[k * n2 + j];
            buf[i * n2 + j] = sum;
        }
}

__device__ void d_rotate_mat(size_t m1,
                             float phi, 
                             char axis,
                             const float *a, 
                             float *buf)
{
	switch (axis)
	{
        case 0:
        {
            float R_x[9] = 
            {
                1,              0,                      0,
                0,              cos(phi),               -1 * sin(phi),
                0,              sin(phi),               cos(phi)
            };
            d_mult_mat(m1, 3, 3, 3, a, R_x, buf);
            break;
        }
        case 1:
        {
            float R_y[9] = 
            {
                cos(phi),                   0,              sin(phi), 
                0,                          1,              0, 
                -1 * sin(phi),              0,              cos(phi)
            };
            d_mult_mat(m1, 3, 3, 3, a, R_y, buf);
            break;
        }
        case 2:
        {
            float R_z[9] = 
            {
                cos(phi),               -1 * sin(phi),              0, 
                sin(phi),               cos(phi),                   0, 
                0,                      0,                          1
            };            
            d_mult_mat(m1, 3, 3, 3, a, R_z, buf);
            break;
        }
	}    
}                             

__device__ void d_trans_mat(size_t m,
                            size_t n, 
                            const float *a, 
                            float *buf)
{
    for (int i = 0; i < m * n; ++i)
    {
        int x = i / m;
        int y = i % m;
        buf[i] = a[n * y + x];
    }
}   

__global__ void render_frame(float A, float B, int offsetx, int offsety, size_t outer, size_t inner, point_t *points)
{

    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (col < inner && row < outer)
    {
        int gindex = row * inner + col; //0 .. 28349

        float theta =   (float) row * THETA_INC;
        float phi =     (float) col * PHI_INC;

        //rotate a point of the torus
        float point[3] = {R2 + R1 * cos(theta), R1 * sin(theta), 0};
        float pointbuf[3][3];
        d_rotate_mat(1, phi, 1, point, pointbuf[0]);
        d_rotate_mat(1, A, 0, pointbuf[0], pointbuf[1]);
        d_rotate_mat(1, B, 2, pointbuf[1], pointbuf[2]);
        //ensure the point is in front of the viewer
        pointbuf[2][2] += DIST;

        point_t temp;
        temp.z_inv = 1 / pointbuf[2][2];
        temp.xp = (int)(COL / 2 + pointbuf[2][0] * ZPRIMEX * temp.z_inv) + offsetx;
        temp.yp = (int)(ROW / 2 - pointbuf[2][1] * ZPRIMEY * temp.z_inv) + offsety;
        
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
        float norm[3] = {cos(theta), sin(theta), 0};
        float normbuf[3][3];
        d_rotate_mat(1, phi, 1, norm, normbuf[0]);
        d_rotate_mat(1, A, 0, normbuf[0], normbuf[1]);
        d_rotate_mat(1, B, 2, normbuf[1], normbuf[2]);
        //dot product of normal vector and light source
        //1 parallel, -1 anti-parallel, 0 perpendicular
        float light[3];
        light[0] = 0;
        light[1] = 1 / sqrt(2.0f);
        light[2] = -1 / sqrt(2.0f);
        float light_transpose[3];
        d_trans_mat(1, 3, light, light_transpose);
        float result[1];
        d_mult_mat(1, 3, 3, 1, normbuf[2], light_transpose, result);

        temp.lum = result[0];

        //capture the results for this point (for this frame) in
        //the synchronized array
        points[gindex] = temp;
    }
}

extern "C" void cuda_render_frame(render_args *input)
{

    point_t *d_points;
    cudaMalloc(&d_points, sizeof(point_t) * input->img.outer * input->img.inner);
    
    dim3 dimBlock(32, 4);

    dim3 dimGrid((int) ceil(input->img.inner * 1.0f / dimBlock.x), (int) ceil(input->img.outer * 1.0f / dimBlock.y));    
    
  
    cudaEvent_t start, stop;
  
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);
    render_frame<<<dimGrid,dimBlock>>>(input->A, input->B,input->offsetx,input->offsety, input->img.outer, input->img.inner, d_points);
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&input->last_frame_time, start, stop);

    
    cudaMemcpy(input->img.points, d_points, sizeof(point_t) * input->img.outer * input->img.inner, cudaMemcpyDeviceToHost);

    cudaFree(d_points);
 
}

extern "C" void cuda_device_reset()
{
    cudaDeviceReset();
}