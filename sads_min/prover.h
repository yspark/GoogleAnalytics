#ifndef __PROVER__
#define __PROVER__

#include <glib.h>

#include "sads_common.h"
#include "typedef.h"

/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/
GHashTable *label_table;
GHashTable *leaf_value_table;



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
#if 0
gsl_vector *get_digest_from_label(gsl_vector *label);
#endif

/** Membership Query **/
MembershipProof *process_membership_query(ULONG nodeid);
GList *build_membership_proof_path(ULONG leaf_nodeid);
void write_membership_proof(MembershipProof *proof, UINT index);

/** Range Query **/
RangeProof *process_range_query(ULONG start_nodeid, ULONG end_nodeid);
void get_inter_nodeids_in_range(GHashTable *nodeids_table, GList *answer_nodeid_list);
void build_range_answer(RangeProof *proof, GList *answer_nodeid_list);
void build_range_proof(RangeProof *proof, GHashTable *nodeid_table);
void write_range_proof(RangeProof *proof, UINT index);

/** Misc **/


/** Benchmark **/
void run_membership_test(char* node_input_filename, int num_query);
void run_range_test(char* node_input_filename, int num_query);


#endif
