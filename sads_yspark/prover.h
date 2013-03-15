#include "typedef.h"

#define UNIVERSE_SIZE 4294967296	// 2^32


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
gsl_vector *root_digest = NULL;

/***********************************************************
*	Functions
************************************************************/
/** Initialize **/
void init_prover(char* filename);
void read_params(char* filename);


/** Update **/
gsl_vector* get_initial_digest(UINT size, BOOL isLeaf);
gsl_vector *get_binary_representation(gsl_vector *digest);

char *encode_vector(gsl_vector  *vector);
gsl_vector *decode_vector_blob(char *buffer);
UINT get_vector_blob_size(char *buffer);
