#ifndef __VERIFIER__
#define __VERIFIER__

#include "sads_common.h"
#include "typedef.h"
#include <glib.h>

/***********************************************************
*	Typedefs
************************************************************/
typedef struct RangeNode_Struct {
	ULONG nodeid;
	UINT answer;
	BOOL isVerified;
	char *label_buffer;
} RangeNode;

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
MembershipProof *read_membership_proof(UINT index);
BOOL verify_membership_proof(MembershipProof *proof);

BOOL verify_radix_leaf(UINT y_leaf, gsl_vector *label);
BOOL verify_radix(gsl_vector *y, gsl_vector *label);


RangeProof *read_range_proof(UINT index);
BOOL verify_range_proof(RangeProof *proof);



/** benchmark **/
void run_membership_test(char* node_input_filename, int num_query);
void run_range_test(char* node_input_filename, int num_query);

#endif
