/********************************************
 * typedef.h
 *
 * This header file should be used by prover.h, verifier.h and sads_common.h only.
 *
 ********************************************/
#ifndef __TYPEDEF__
#define __TYPEDEF__

#define UINT unsigned int
#define INT	int
#define ULONG unsigned long long
#define LONG long long
#define BOOL int

#ifdef SADS_DEBUG
#define DEBUG_TRACE(X) printf X
#else
#define DEBUG_TRACE(X) do {} while(0)
#endif

typedef enum {
	NOT_SUBTREE,
	IDENTICAL_NODES,
	RIGHT_SUBTREE,
	LEFT_SUBTREE
} SUBTREE_TYPE;

typedef struct MembershipProof_Struct {
	ULONG query_nodeid;
	UINT answer;
	//UINT nodeid_num;
	//ULONG *nodeid_list;
	char **label_list;
} MembershipProof;

typedef struct RangeProof_Struct {
	/** Range query: start nodeid, end nodeid **/
	ULONG start_nodeid;
	ULONG end_nodeid;

	/** Answer */
	UINT num_answer_nodeid;
	ULONG *answer_nodeid_list;		// nodeid_list[num_answer_nodeid]
	UINT *answer_list;				// answer_list[num_answer_nodeid]

	/** Proof */
	UINT num_proof_nodeid;
	ULONG *proof_nodeid_list;				// nodeid_list[num_all_nodeid]
	char **proof_label_buffer_list;		// *label_buffer_list[num_all_nodeid]
} RangeProof;

#endif

