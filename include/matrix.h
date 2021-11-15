#ifndef MATRIX_H
#define MATRIX_H

enum rotationAxis {aX = 0, aY = 1, aZ = 2};

extern void mult_mat(size_t m1,
                     size_t n1, 
                     size_t m2, 
                     size_t n2,
                     const float (*a)[n1], 
                     const float (*b)[n2], 
                     float (*buf)[n2]);

extern void rotate_mat(size_t m1, 
                       float phi, 
                       enum rotationAxis axis, 
                       const float (*a)[3], 
                       float (*buf)[3]);

extern void trans_mat(size_t m, 
                      size_t n, 
                      const float (*a)[n], 
                      float (*buf)[m]);

extern float dot_mat(size_t n, 
                     const float (*a)[n], 
                     const float (*b)[n]);

#endif