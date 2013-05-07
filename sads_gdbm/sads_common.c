#include <stdio.h>
#include <math.h>

#include "sads_common.h"

/** initialize **/
void read_params(char* filename) {
	FILE *fp = fopen(filename, "r");

	fscanf(fp, "%d", &k);
	fscanf(fp, "%d", &n);
	fscanf(fp, "%d", &q);
	fscanf(fp, "%d", &mu);
	fscanf(fp, "%d", &beta);

	m = mu / 2;
	log_q_ceil = ceil(log2(q));

	L = gsl_matrix_alloc(k, m);
	R = gsl_matrix_alloc(k, m);

	gsl_matrix_fscanf(fp, L);
	gsl_matrix_fscanf(fp, R);

	fclose(fp);
}


/** misc **/
gsl_vector *get_initial_digest(BOOL isLeaf) {

	gsl_vector *digest = gsl_vector_alloc(DIGEST_LEN);
	int i = 0;

	if(isLeaf) {
	  for (i = 0; i < DIGEST_LEN; i++)
			gsl_vector_set(digest, i, 1);
	}
	else {
	  for (i = 0; i < DIGEST_LEN; i++)
			gsl_vector_set(digest, i, 0);
	}

	DEBUG_TRACE(("get_initial_digest(%d)\n", isLeaf));

	return digest;
}


gsl_vector *get_initial_label(BOOL isLeaf) {
#if 0
	printf("***get_initial_label\n");
	gsl_vector_fprintf(stdout, get_binary_representation(get_initial_digest(isLeaf)), "%f");
#endif

	return get_binary_representation(get_initial_digest(isLeaf));
}


gsl_vector *get_binary_representation(gsl_vector *digest) {

	gsl_vector *binary = gsl_vector_alloc((digest->size) * log_q_ceil);
	int i = 0, j = 0, ele = 0;

	DEBUG_TRACE(("get_binary_representation\n"));
	//DEBUG_TRACE(("digest(%d), binary(%d), log_q_ceil(%d)\n", (UINT)digest->size, (UINT)binary->size, log_q_ceil));

	//printf("%lu, %lu, %lu\n", sizeof(size_t), sizeof(gsl_block), sizeof(int));
	//printf("%f, %f, %f\n", *(digest->data), *(digest->data+32), *(digest->data+16));

	for(i = 0; i < digest->size; i++) {
		ele = (UINT)gsl_vector_get(digest, i);


		for(j = log_q_ceil -1; j >= 0; j--) {

			gsl_vector_set(binary, (i * log_q_ceil) + j, ele & 1);
			ele = ele >> 1;
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

void mod_q(gsl_vector *vector, UINT q) {
#if 0
	/* Check if max(vector) > q */
	if(gsl_vector_max(vector) < q) {
		return;
	}
#endif

	//DEBUG_TRACE(("mod_q\n"));

	int i = 0;
	for(i=0; i< vector->size; i++) {
		if(vector->data[i] >= q) {
			vector->data[i] = (ULONG)vector->data[i] % q;
		}
	}

	return;
}


UINT get_tree_height() {
	return ceil(log2(UNIVERSE_SIZE)) + 1;
}


char *encode_vector(gsl_vector  *vector) {

	if(vector == NULL) return NULL;

	int size = ELEMENT_LEN*(vector->size);
	int len = 0;
	char *buffer = malloc(size);

	memcpy(buffer+len, vector->data, ELEMENT_LEN*(vector->size));
	len += ELEMENT_LEN*(vector->size);

	DEBUG_TRACE(("encode_vector: %d, %d\n", size, len));

	return buffer;
}


gsl_vector *decode_vector_buffer(char *buffer, UINT size) {
	/*
	int k;
	int j;
	*/

	if(buffer == NULL) return NULL;
	int vec_len = (UINT)size / (UINT)ELEMENT_LEN;

	gsl_vector *vector = gsl_vector_alloc(vec_len);

	/*
	for(j=0; j<LABEL_LEN; j++) {
		double a;
		memcpy(&a, buffer+j*sizeof(double), sizeof(double));

		printf("%f\n", a);
	}
	*/

	memcpy(vector->data, buffer, size);

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



/** Benchmark **/
ULONG *read_node_input(char *node_input_filename, UINT *num_nodes) {
	FILE *fp = fopen(node_input_filename, "r");
	UINT i = 0;
	ULONG *node_list = NULL;
	char ip_addr[16];

	fscanf(fp, "%d", num_nodes);

	node_list = malloc(*num_nodes * sizeof(ULONG));

	for(i=0; i<*num_nodes; i++) {
		fscanf(fp, "%s", ip_addr);
		node_list[i] = get_nodeid_from_string(ip_addr);
	}

	fclose(fp);

	return node_list;
}





