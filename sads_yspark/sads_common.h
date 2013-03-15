#ifndef __SADS_COMMON__
#define __SADS_COMMON__

#include <gsl/gsl_vector.h>

#include "typedef.h"


#define UNIVERSE_SIZE 4294967296	// 2^32
#define UNIVERSE_SIZE_IN_BIT 32	// 2^32

#ifdef SADS_DEBUG
#define DEBUG_TRACE(X) printf X
#else
#define DEBUG_TRACE(X) do {} while(0)
//#define DEBUG_TRACE(X) printf("aa")
#endif


/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/

/***********************************************************
*	Functions
************************************************************/




#endif
