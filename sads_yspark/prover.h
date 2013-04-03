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


//gsl_vector *get_partial_label(ULONG nodeid, ULONG wrt_nodeid);
gsl_vector *update_partial_label(ULONG nodeid, ULONG wrt_nodeid);
void update_node_label(ULONG nodeid, gsl_vector *partial_label);

void verify_label(ULONG nodeid, gsl_vector *label);
gsl_vector *get_digest_from_label(gsl_vector *label);


/** Query **/
Proof *process_membership_query(ULONG nodeid);
void build_proof_path(ULONG child_of_root_nodeid, ULONG leaf_nodeid, UINT nodeid_num, ULONG *nodeid_list);
//void build_proof_path(ULONG ancester_nodeid, ULONG decendent_nodeid, UINT *temp_nodeid_num, ULONG *temp_nodeid_list);


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
