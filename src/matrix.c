#include <stddef.h>
#include <math.h>
#include "matrix.h"

void mult_mat(size_t      m1,
              size_t      n1,
              size_t      m2,
              size_t      n2,
              const float (*a)[n1],
              const float (*b)[n2],
              float       (*buf)[n2])
{
        float sum;
        for (size_t i = 0; i < m1; ++i)
                for (size_t j = 0; j < n2; ++j)
                {
                        sum = 0;
                        for (size_t k = 0; k < n1; ++k)
                                sum += a[i][k] * b[k][j];
                        buf[i][j] = sum;
                }
}

void rotate_mat(size_t      m1,
                float       phi,
                enum        rotationAxis axis,
                const float (*a)[3],
                float       (*buf)[3])
{
        switch (axis)
        {
                case aX:
                        {
                                float R_x[3][3] =
                                {
                                        {1, 0,        0},
                                        {0, cos(phi), -1 * sin(phi)},
                                        {0, sin(phi), cos(phi)}
                                };
                                mult_mat(m1, 3, 3, 3, a, R_x, buf);
                                break;
                        }
                case aY:
                        {
                                float R_y[3][3] =
                                {
                                        {cos(phi),      0, sin(phi)},
                                        {0,             1, 0},
                                        {-1 * sin(phi), 0, cos(phi)}
                                };
                                mult_mat(m1, 3, 3, 3, a, R_y, buf);
                                break;
                        }
                case aZ:
                        {
                                float R_z[3][3] =
                                {
                                        {cos(phi), -1 * sin(phi), 0},
                                        {sin(phi), cos(phi),      0},
                                        {0,        0,             1}
                                };
                                mult_mat(m1, 3, 3, 3, a, R_z, buf);
                                break;
                        }
        }
}

void trans_mat(size_t      m,
               size_t      n,
               const float (*a)[n],
               float       (*buf)[m])
{
        for (size_t i = 0; i < n; ++i)
                for (size_t j = 0; j < m; ++j)
                        buf[i][j] = a[j][i];
}

float dot_mat(size_t      n,
              const float (*a)[n],
              const float (*b)[n])
{
        float btranspose[n][1], result[1][1];

        trans_mat(1, n, b, btranspose);
        mult_mat(1, n, n, 1, a, btranspose, result);

        return result[0][0];
}
