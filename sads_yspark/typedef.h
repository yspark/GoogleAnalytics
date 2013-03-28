#ifndef __TYPEDEF__

#define __TYPEDEF__

#define UINT unsigned int
#define INT	int
#define ULONG unsigned long long
#define LONG long long
#define BOOL int

typedef enum {
	NOT_SUBTREE,
	IDENTICAL_NODES,
	RIGHT_SUBTREE,
	LEFT_SUBTREE
} SUBTREE_TYPE;

typedef struct Proof_Struct {
	UINT nodeid_num;
	ULONG *nodeid_list;
	char **label_list;
} Proof;


#endif

