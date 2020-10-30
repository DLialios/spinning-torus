//standard matrix multiplication
extern void mult_mat(size_t m1, size_t n1, size_t m2, size_t n2,
					 const float (*a)[n1], const float (*b)[n2], float (*buf)[n2]);

//perform a rotation about an axis
//0 for x, 1 for y, 2 for z
extern void rotate_mat(size_t m1, float phi, char axis, const float (*a)[3], float (*buf)[3]);

//transpose a matrix
extern void trans_mat(size_t m, size_t n, const float (*a)[n], float (*buf)[m]);

//dot product
extern float dot_mat(size_t n, const float (*a)[n], const float (*b)[n]);

//print matrix values
//name is used in output
extern void print_mat(size_t m, size_t n, const float (*a)[n], const char *name);