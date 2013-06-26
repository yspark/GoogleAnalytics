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
SVector get_partial_digest(ULONG nodeid, ULONG wrt_nodeid);
void update_root_digest(SVector partial_digest);

/** verify **/
MembershipProof *read_membership_proof(UINT index);
BOOL verify_membership_proof(MembershipProof *proof);

BOOL verify_radix_leaf(UINT y_leaf, SVector label);
BOOL verify_radix(SVector y, SVector label);


RangeProof *read_range_proof(UINT index);
BOOL verify_range_proof(RangeProof *proof);



/** benchmark **/
void run_membership_test(char* node_input_filename, UINT num_query);
void run_range_test(char* node_input_filename, UINT num_query);

#endif
