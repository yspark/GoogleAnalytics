#include <stdio.h>
//#include <math.h>

#include "sads_common.h"
#include <iostream>


using namespace Eigen;


/***********************************************************
*	Parameters, Matrices, Vectors
************************************************************/
UINT k;										// Security Parameter
UINT m;
ULONG q;										// The modulus
UINT log_q;

SMatrix L, R;
SVector root_digest;


/** initialize **/
void read_params(char* filename) {
	FILE *fp = fopen(filename, "r");
	int retVal;

	retVal = fscanf(fp, "%u", &k);
	check_retVal(retVal);
	retVal = fscanf(fp, "%u", &m);
	check_retVal(retVal);
	retVal = fscanf(fp, "%llu", &q);
	check_retVal(retVal);
	retVal = fscanf(fp, "%u", &log_q);
	check_retVal(retVal);

	L.resize(k,m);
	R.resize(k,m);

	for(UINT i=0; i<k; i++) {
		for(UINT j=0; j<m; j++) {
			retVal = fscanf(fp, "%llu", &L(i,j));
			check_retVal(retVal);
		}
	}

	for(UINT i=0; i<k; i++) {
		for(UINT j=0; j<m; j++) {
			retVal = fscanf(fp, "%llu", &R(i,j));
			check_retVal(retVal);
		}
	}

	fclose(fp);
}

/** misc **/
SVector get_initial_digest(BOOL isLeaf) {

	SVector digest;

	if(isLeaf) {
		digest.setOnes(DIGEST_LEN, 1);
	}
	else {
		digest.setZero(DIGEST_LEN, 1);
	}

	DEBUG_TRACE(("get_initial_digest(%d)\n", isLeaf));

	return digest;
}

SVector get_initial_label(BOOL isLeaf) {
	SVector label;

	if(isLeaf) {
		SVector digest = get_initial_digest(isLeaf);
		label = get_binary_representation(digest);

		digest.resize(0,0);
	}
	else {
		label.setZero(LABEL_LEN,1);
	}

	return label;
}

SVector get_binary_representation(SVector digest) {

	SVector binary;
	ULONG ele = 0;
	UINT digest_len = (UINT)digest.rows();

	DEBUG_TRACE(("get_binary_representation\n"));
	//DEBUG_TRACE(("digest(%d), binary(%d), log_q_ceil(%d)\n", (UINT)digest->size, (UINT)binary->size, log_q_ceil));

	//printf("%lu, %lu, %lu\n", sizeof(size_t), sizeof(gsl_block), sizeof(int));
	//printf("%f, %f, %f\n", *(digest->data), *(digest->data+32), *(digest->data+16));

	binary.setZero(digest.rows() * log_q, 1);

	for(UINT i = 0; i < digest_len; i++) {
		ele = digest(i,0);

		for(UINT j = log_q -1; j >= 0; j--) {
			binary((i * log_q) + j, 0) = (ele & (ULONG)1);
			ele = ele >> 1;

			if(j==0) break;
		}
	}

	return binary;
}


/** misc **/
ULONG get_nodeid_from_string(char *ip_addr) {
	ULONG nodeid = 0;
	int i = 3;
	char *token;

	DEBUG_TRACE(("IP conversion: %s -> ", ip_addr));

	for(i = 3; i >= 0;i--, ip_addr = NULL) {
		token = strtok(ip_addr, ".");
		if(token == NULL) {
			printf("Error get_nodeid_from_string(%s), %d", ip_addr, i);
			exit(1);
		}

		nodeid = nodeid << 8;
		nodeid += (ULONG)atoi(token);
	}

	nodeid += (ULONG)((ULONG)1 << 32);

	DEBUG_TRACE(("%llu\n", nodeid));

	return nodeid;
}

int get_number_of_bits(ULONG nodeid) {
	UINT i = 0;

	for(i=UNIVERSE_SIZE_IN_BIT; i>=0; i--) {
		if(nodeid & ((ULONG)1 << i))
			break;
	}
	return(i+1);

	//return floor(log2(nodeid)) + 1;
}


SVector mod_q(SVector vector, ULONG q) {
	DEBUG_TRACE(("mod_q\n"));

	for(UINT i=0; i< (UINT)vector.rows(); i++) {
		if(vector(i,0) >= q) {
			//printf("%llu -> ", (ULONG)vector->data[i]);
			vector(i,0) = (ULONG)((ULONG)vector(i,0) % (ULONG)q);
			//printf("%llu\n", (ULONG)vector->data[i]);
		}
	}

	return vector;
}

#if 0
UINT get_tree_height() {
	return ceil(log2(UNIVERSE_SIZE)) + 1;
}
#endif


char *encode_vector(SVector  vector) {

	if(vector.rows() == 0) return NULL;

	int size = ELEMENT_LEN*(vector.rows());
	int len = 0;
	char *buffer = (char *)malloc(size);

	memcpy(buffer+len, &vector(0,0), ELEMENT_LEN*(vector.rows()));
	len += ELEMENT_LEN*(vector.rows());

	DEBUG_TRACE(("encode_vector: %d, %d\n", size, len));

	return buffer;
}

SVector decode_vector_buffer(char *buffer, UINT size) {
	SVector vector;
	UINT vec_len = (UINT)size / (UINT)ELEMENT_LEN;

	if(buffer == NULL) {
		vector.resize(0,0);
		return vector;
	}

	vector.resize(vec_len, 1);

	/*
	for(j=0; j<LABEL_LEN; j++) {
		double a;
		memcpy(&a, buffer+j*sizeof(double), sizeof(double));

		printf("%f\n", a);
	}
	*/

	memcpy(&vector(0,0), buffer, size);

	/*
	for(k=0; k<LABEL_LEN; k++)
		printf("%f", vector->data[k]);


	printf("vec_len:%d\n", vec_len);
	printf("max:%f\n", gsl_vector_max(vector));
	*/
	//printf("element_len:%d\n", *

	//printf("size:%u, vec_len:%u, ele_len:%f, max:%f\n", size, ceil(size / ELEMENT_LEN), ELEMENT_LEN, gsl_vector_max(vector));

	return vector;
}


void free_membership_proof(MembershipProof *proof) {
	UINT i;

	free(proof->proof_nodeid_list);

	for(i=0; i<proof->num_proof_nodeid; i++) {
		free(proof->proof_label_list[i]);
	}

	free(proof->proof_label_list);
	free(proof);
}


/** Benchmark **/
ULONG *read_node_input(char *node_input_filename, UINT *num_nodes) {
	FILE *fp = fopen(node_input_filename, "r");
	ULONG *node_list = NULL;
	char ip_addr[16];

	if(!fscanf(fp, "%d", num_nodes)) {
		printf("read_node_input(): scanf error1\n");
	}

	node_list = (ULONG*)malloc((*num_nodes) * sizeof(ULONG));

	for(UINT i=0; i<(*num_nodes); i++) {
		if(!fscanf(fp, "%s", ip_addr)) {
			printf("read_node_input(): scanf error2\n");
		}
		node_list[i] = get_nodeid_from_string(ip_addr);
	}

	fclose(fp);

	return node_list;
}




