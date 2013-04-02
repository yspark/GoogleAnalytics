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

typedef struct Proof_Struct {
	ULONG query_nodeid;
	UINT answer;
	//UINT nodeid_num;
	//ULONG *nodeid_list;
	char **label_list;
} Proof;


#endif

