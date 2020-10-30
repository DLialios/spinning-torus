//output size
static const int ROW = 24;
static const int COL = 60;

//radius of circle
static const float R1 = 1;
//radius of torus
static const float R2 = 2;

//there's an optimization problem for computing these vals
//such that the resultant projection does not produce an
//out-of-bounds array index...
//...or just snap the points to the edges when needed
static const int ZPRIMEX = 26;
static const int ZPRIMEY = 10;

//lower values correspond to more points
static const float THETA_INC = 0.07;
static const float PHI_INC = 0.02;

//x-axis rotation speed
static const float A_INC = 0.05;
//z-axis rotation speed
static const float B_INC = 0.03;

//~33ms per frame for 30fps
static const int SLEEPTIME = 33333;

//symbols for luminance values
static const char LIGHTSYM[] = ",:;-~=*#&$@";