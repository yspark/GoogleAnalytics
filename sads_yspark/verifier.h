#ifndef __VERIFIER__
#define __VERIFIER__

#include "sads_common.h"
#include "typedef.h"
#include "sads_mysql.h"		// necessary for TRUE/FALSE


/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/


/***********************************************************
*	Functions
************************************************************/
/** initialize **/
void init_verifier(char* param_filename);

/** update **/
void update_verifier(ULONG nodeid);
gsl_vector *get_partial_digest(ULONG nodeid, ULONG wrt_nodeid);
void update_root_digest(gsl_vector *partial_digest);

/** verify **/
BOOL verify_membership_proof(MembershipProof *proof);

BOOL verify_radix_leaf(UINT y_leaf, gsl_vector *label);
BOOL verify_radix(gsl_vector *y, gsl_vector *label);


/** benchmark **/
void run_test(char* node_input_filename);
MembershipProof *read_membership_proof(UINT index);

#endif
