#ifndef __SADS_COMMON__
#define __SADS_COMMON__

#include "typedef.h"

#define UNIVERSE_SIZE 4294967296	// 2^32
#define UNIVERSE_SIZE_IN_BIT 32		// 2^32

#define ELEMENT_LEN sizeof(ULONG)
#define LABEL_LEN	m
#define DIGEST_LEN	k
#define LABEL_BUFFER_LEN (LABEL_LEN*ELEMENT_LEN)
#define DIGEST_BUFFER_LEN (DIGEST_LEN*ELEMENT_LEN)


#define check_retVal(retVal) { if(!retVal) {printf("invalid retVal\n"); exit(-1);}}




/***********************************************************
*	Functions
************************************************************/
/** initialize **/
void read_params(char* filename);


/** update **/
SVector get_initial_digest(BOOL isLeaf);
SVector get_initial_label(BOOL isLeaf);
SVector get_binary_representation(SVector digest);


/** misc **/
ULONG get_nodeid_from_string(char *ip_addr);
int get_number_of_bits(ULONG nodeid);
SVector mod_q(SVector vector, ULONG q);
UINT get_tree_height();

char *encode_vector(SVector vector);
SVector decode_vector_buffer(char *buffer, UINT size);

void free_membership_proof(MembershipProof *proof);


/** Benchmark **/
ULONG *read_node_input(char *node_input_filename, UINT *num_nodes);



#endif
