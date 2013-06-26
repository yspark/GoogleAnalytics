#ifndef __SADS_COMMON__
#define __SADS_COMMON__

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

#include <string.h>

#include "typedef.h"

#define UNIVERSE_SIZE 4294967296	// 2^32
#define UNIVERSE_SIZE_IN_BIT 32		// 2^32

#define ELEMENT_LEN sizeof(double)
#define LABEL_LEN	m
#define DIGEST_LEN	k
#define LABEL_BUFFER_LEN (LABEL_LEN*ELEMENT_LEN)
#define DIGEST_BUFFER_LEN (DIGEST_LEN*ELEMENT_LEN)


#define check_retVal(retVal) { if(!retVal) {printf("invalid retVal\n"); exit(-1);}}

/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/
UINT k;										// Security Parameter
UINT n;										// Upper bound on the size of the stream

UINT q;										// The modulus
UINT mu;
UINT beta;

UINT m;
UINT log_q_ceil;

gsl_matrix *L, *R;
gsl_vector *root_digest;


/***********************************************************
*	Functions
************************************************************/
/** initialize **/
void read_params(char* filename);


/** update **/
gsl_vector* get_initial_digest(BOOL isLeaf);
gsl_vector* get_initial_label(BOOL isLeaf);
gsl_vector *get_binary_representation(gsl_vector *digest);


/** misc **/
ULONG get_nodeid_from_string(char *ip_addr);
int get_number_of_bits(ULONG nodeid);
void mod_q(gsl_vector *vector, UINT q);
UINT get_tree_height();

char *encode_vector(gsl_vector  *vector);
gsl_vector *decode_vector_buffer(char *buffer, UINT size);

void free_membership_proof(MembershipProof *proof);


/** Benchmark **/
ULONG *read_node_input(char *node_input_filename, UINT *num_nodes);



#endif
