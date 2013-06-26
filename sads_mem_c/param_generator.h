#include "typedef.h"

/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/
UINT k = 0;										// Security Parameter
UINT n = 0;										// Upper bound on the size of the stream

UINT q = 0;										// The modulus 
UINT mu = 0;									
UINT beta = 0;

UINT m = 0;
UINT log_q_ceil = 0;

gsl_matrix *L = NULL, *R = NULL;


/***********************************************************
*	Functions
************************************************************/
void initialize(UINT k, UINT n);
void init_LR();
