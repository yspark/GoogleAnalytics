#include "typedef.h"
#include <Eigen/Dense>
using namespace Eigen;

/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/
UINT k = 0;										// Security Parameter
UINT m = 0;
ULONG q = 0;										// The modulus
UINT log_q = 0;

//ULONG n = 0;										// Upper bound on the size of the stream

//gsl_matrix *L = NULL, *R = NULL;
SMatrix L, R;


/***********************************************************
*	Functions
************************************************************/
void initialize(int securityLevel);
void init_LR();
