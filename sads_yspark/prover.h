#ifndef __PROVER__
#define __PROVER__


#include "sads_common.h"
#include "typedef.h"

/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/


/***********************************************************
*	Functions
************************************************************/
/** Initialize **/
void init_prover(char* filename);
void read_params(char* filename);


/** Update **/
void update_prover(ULONG nodeid);

UINT update_leaf(ULONG nodeid);
void update_path_labels(ULONG nodeid);

gsl_vector *update_partial_label(ULONG nodeid, ULONG wrt_nodeid);
void update_node_label(ULONG nodeid, gsl_vector *partial_label);

void verify_label(ULONG nodeid, gsl_vector *label);
gsl_vector *get_digest_from_label(gsl_vector *label);


/** Query **/
MembershipProof *process_membership_query(ULONG nodeid);
void build_membership_proof_path(ULONG child_of_root_nodeid, ULONG leaf_nodeid, UINT nodeid_num, ULONG *nodeid_list);
void write_membership_proof(MembershipProof *proof, UINT index);


RangeProof *process_range_query(ULONG start_nodeid, ULONG end_nodeid);
//UINT build_range_proof_path(ULONG start_nodeid, ULONG end_nodeid, ULONG *nodeid_list);
void build_range_proof_path(RangeProof *proof);
void write_range_proof(RangeProof *proof);



/** Misc **/
#if 0
ULONG get_nodeid(UINT ip_addr);

UINT get_label_buffer_len();
UINT get_digest_buffer_len();

UINT get_vector_blob_size(char *buffer);
#endif


//SUBTREE_TYPE identifySubtree(ULONG ancester_nodeid, ULONG decendent_nodeid);


/** Benchmark **/
void run_test(char* node_input_filename);

#endif
