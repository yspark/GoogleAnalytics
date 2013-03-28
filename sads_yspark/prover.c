#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

#include "prover.h"
#include "sads_mysql.h"
#include "sads_common.h"

char* temp_buffer;


/********************************************************
*
*	initialize()
*
*********************************************************/
void initialize(char* filename) {
	read_params(filename);
	
	root_digest = get_initial_digest(k, FALSE);
}


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




/********************************************************
*
*	update()
*
*********************************************************/
UINT update_leaf(ULONG nodeid) {
	UINT curr_count = 0;

	DEBUG_TRACE(("update_leaf(%llu)\n", nodeid));

	curr_count = smysql_get_leaf_val(nodeid);

	if(curr_count == 0) {
		smysql_add_leaf(nodeid);
	}
	else {
		smysql_update_leaf(nodeid, curr_count+1);
	}

	return (curr_count+1);
}


void update_path_labels(ULONG nodeid) {
	ULONG child_of_root_nodeid = nodeid >> (get_number_of_bits(nodeid)-2);

	DEBUG_TRACE(("update_path_labels %llu - %llu\n", child_of_root_nodeid, nodeid));

	update_partial_label(child_of_root_nodeid, nodeid);
}


gsl_vector *update_partial_label(ULONG nodeid, ULONG wrt_nodeid) {
	gsl_vector *partial_digest = NULL, *partial_label = NULL;
	gsl_vector *label = NULL;
	int bit_shift_count =  get_number_of_bits(wrt_nodeid) - get_number_of_bits(nodeid);

	DEBUG_TRACE(("update_partial_label(): nodeid(%llu), wrt_nodeid(%llu), bit_shift(%d)\n", nodeid, wrt_nodeid, bit_shift_count));

	if(bit_shift_count < 0) {
		printf("Error get_partial_label(%llu: %llu)\n", nodeid, wrt_nodeid);
		exit(1);
	}
	/* reaches a leaf node */
	else if(bit_shift_count == 0) {
		if(nodeid != wrt_nodeid) {
			printf("Error nodeid(%llu) == wrt_nodeid(%llu)", nodeid, wrt_nodeid);
			exit(1);
		}
		DEBUG_TRACE(("Reached a leaf node\n"));
		label = get_binary_representation(get_initial_digest(DIGEST_LEN, TRUE));
		//printf("label of leaf: %u\n", (UINT)label->size);
		return label;
	}

	partial_digest = gsl_vector_alloc(DIGEST_LEN);
	partial_label = gsl_vector_alloc(LABEL_LEN);
	//printf("partial label: %u\n", (UINT)partial_label->size);

	/* Right child */
	if(wrt_nodeid & ((ULONG)1 << (bit_shift_count-1))) {
		gsl_blas_dgemv(CblasNoTrans,\
				1.0, \
				R, \
				update_partial_label((nodeid<<1)+(ULONG)1, wrt_nodeid), \
				0.0, \
				partial_digest);
	}
	/* Left child */
	else {
		gsl_blas_dgemv(CblasNoTrans,\
				1.0, \
				L, \
				update_partial_label(nodeid<<1, wrt_nodeid), \
				0.0, \
				partial_digest);
	}

	partial_label = get_binary_representation(partial_digest);
	mod_q(partial_label, q);

	update_node_label(nodeid, partial_label);

	return partial_label;
}


void update_node_label(ULONG nodeid, gsl_vector *partial_label) {
	gsl_vector *label = NULL;
	char *label_buffer = NULL;

	DEBUG_TRACE(("update_node_label(): nodeid(%llu)\n", nodeid));

	label = decode_vector_blob(smysql_get_node_label(nodeid), LABEL_LEN);

	if(label == NULL) {
		label_buffer = encode_vector(partial_label);
		smysql_add_inter_node(nodeid, label_buffer, LABEL_BUFFER_LEN);
	}
	else {

		//printf("label size:%d, partial_label:%d\n", (UINT)label->size, (UINT)partial_label->size);

		gsl_vector_add(label, partial_label);
		mod_q(label, q);

		label_buffer = encode_vector(label);
		smysql_update_inter_node(nodeid, label_buffer, LABEL_BUFFER_LEN);
	}
}


gsl_vector *get_initial_digest(UINT size, BOOL isLeaf) {

	gsl_vector *digest = gsl_vector_alloc(size);
	int i = 0;

	if(isLeaf) {
	  for (i = 0; i < size; i++)
			gsl_vector_set(digest, i, 1);
	}
	else {
	  for (i = 0; i < size; i++)
			gsl_vector_set(digest, i, 0);
	}

	DEBUG_TRACE(("get_initial_digest(%u, %d)\n", size, isLeaf));

	return digest;
}


gsl_vector *get_binary_representation(gsl_vector *digest) {

	gsl_vector *binary = gsl_vector_alloc((digest->size) * log_q_ceil);
	int i = 0, j = 0, ele = 0;

	DEBUG_TRACE(("get_binary_representation\n"));
	DEBUG_TRACE(("digest(%d), binary(%d), log_q_ceil(%d)\n", (UINT)digest->size, (UINT)binary->size, log_q_ceil));

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


/********************************************************
*
*	query()
*
*********************************************************/
Proof *process_membership_query(ULONG nodeid) {
	Proof *proof;
	ULONG temp_nodeid_list[get_tree_height()*2];
	UINT temp_nodeid_num = 0;
	UINT i = 0;

	/** 1. get nodeids on the path.  Build proof->nodeid_num, proof->nodeid_list **/
	ULONG child_of_root_nodeid = nodeid >> (get_number_of_bits(nodeid)-2);

	DEBUG_TRACE(("process_membership_query(%llu)\n", nodeid));

	build_proof_path(child_of_root_nodeid, nodeid, &temp_nodeid_num, temp_nodeid_list);
	DEBUG_TRACE(("build_proof_path done.\n"));

	proof = malloc(sizeof(Proof));
	proof->nodeid_num = temp_nodeid_num;
	proof->nodeid_list = malloc(sizeof(ULONG)*get_tree_height()*2);
	memcpy(proof->nodeid_list, temp_nodeid_list, sizeof(ULONG)*get_tree_height()*2);


	/** 2. get labels of nodes included in proof->nodeid->list **/
	proof->label_list = malloc(proof->nodeid_num * sizeof(char *));
	for(i=0; i < proof->nodeid_num; i++) {
		//proof->label_list[i] = malloc(get_label_buffer_len());
		proof->label_list[i] = smysql_get_node_label(proof->nodeid_list[i]);
	}

	/** 3. return proof **/
	return proof;
}

void build_proof_path(ULONG ancester_nodeid, ULONG decendent_nodeid, UINT *temp_nodeid_num, ULONG *temp_nodeid_list) {
	UINT curr_nodeid_num = *temp_nodeid_num;

	DEBUG_TRACE(("build_proof_path(%llu, %llu, %u).\n", ancester_nodeid, decendent_nodeid, *temp_nodeid_num));

	/** Add ancester_nodeid and its sibling to the list **/
	temp_nodeid_list[curr_nodeid_num++] = ancester_nodeid;
	temp_nodeid_list[curr_nodeid_num++] = (ancester_nodeid ^ (ULONG)1);

	*temp_nodeid_num = curr_nodeid_num;

	switch(identifySubtree(ancester_nodeid, decendent_nodeid)){
		case NOT_SUBTREE:
			printf("Error buildProofPath(%llu, %llu)\n", ancester_nodeid, decendent_nodeid);
			exit(1);
		case IDENTICAL_NODES:
			break;
		case RIGHT_SUBTREE:
			build_proof_path((ancester_nodeid<<1) + (ULONG)1, decendent_nodeid, temp_nodeid_num, temp_nodeid_list);
			break;
		case LEFT_SUBTREE:
			build_proof_path(ancester_nodeid<<1, decendent_nodeid, temp_nodeid_num, temp_nodeid_list);
			break;
	}
}

/********************************************************
*
*	misc.
*
*********************************************************/
int get_number_of_bits(ULONG nodeid) {
	return floor(log2(nodeid)) + 1;
}

ULONG get_nodeid(UINT ip_addr) {
	return ((ULONG)1 << 32) + (ULONG)ip_addr;
}

ULONG get_nodeid_from_string(char *ip_addr) {
	ULONG nodeid = 0;
	int i = 3;
	char *token;

	DEBUG_TRACE(("IP conversion: %s -> ", ip_addr));

	for(i = 3; ;i--, ip_addr = NULL) {
		token = strtok(ip_addr, ".");
		if(token == NULL) {
			if(i == -1) break;
			else {
				printf("Error get_nodeid_from_string(%s), %d", ip_addr, i);
				exit(1);
			}
		}

		nodeid += (atoi(token) << (8*i));
	}
	nodeid += (ULONG)1 << 32;

	DEBUG_TRACE(("%llu\n", nodeid));

	return nodeid;
}

UINT get_label_buffer_len() {
	return LABEL_LEN * ELEMENT_LEN;
}

UINT get_digest_buffer_len() {
	return DIGEST_LEN * ELEMENT_LEN;
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


gsl_vector *decode_vector_blob(char *buffer, UINT size) {
	if(buffer == NULL) return NULL;

	gsl_vector *vector = gsl_vector_alloc(size);
	
	memcpy(vector->data, buffer, LABEL_BUFFER_LEN);
	
	return vector;	
}

void mod_q(gsl_vector *vector, UINT q) {
	/* Check if max(vector) > q */
	if(gsl_vector_max(vector) < q) {
		return;
	}

	DEBUG_TRACE(("mod_q\n"));

	int i = 0;
	for(i=0; i< vector->size; i++) {
		if(vector->data[i] >= q) {
			vector->data[i] = (ULONG)vector->data[i] % q;
		}
	}

	return;
}

UINT get_tree_height() {
	return ceil(log2(UNIVERSE_SIZE));
}

/***********************************************
 * return -1: error (decendend_nodeid is not in any subtrees of ancester_nodeid
 * return 0: identical nodeids
 * return 1: right subtree
 * return 2: left subtree
************************************************/
SUBTREE_TYPE identifySubtree(ULONG ancester_nodeid, ULONG decendent_nodeid) {
	int bit_shift_count =  get_number_of_bits(decendent_nodeid) - get_number_of_bits(ancester_nodeid);

	if(bit_shift_count < 0) {
		return NOT_SUBTREE;
	}
	else if(bit_shift_count == 0) {
		if(ancester_nodeid == decendent_nodeid)
			return IDENTICAL_NODES;
	}
	else if((decendent_nodeid >> bit_shift_count) != ancester_nodeid) {
		return NOT_SUBTREE;
	}

	if(decendent_nodeid & ((ULONG)1 << (bit_shift_count-1))) {
		return RIGHT_SUBTREE;
	}
	else {
		return LEFT_SUBTREE;
	}

}


/********************************************************
*
*	temporary test functions
*
*********************************************************/
void testMatrix() {
	gsl_matrix *A = gsl_matrix_alloc(k, m);

	printf("testMatrix\n");
	printf("%d %d\n", (int)L->size1, (int)L->size2);
	printf("%d %d\n", (int)R->size1, (int)R->size2);
	
	gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, L, R, 0.0, A);
	printf("%d %d\n", (int)A->size1, (int)A->size2);
	
	gsl_matrix_fprintf(stdout, A, "%f");


}

void test_mysql() {
		/*
		////////////////////////////////
		gsl_vector *digest2 = NULL, *label2 = NULL;
		int digest_buffer_len = 0, label_buffer_len = 0;
		
		digest_buffer = encode_vector(digest);		
		digest2 = decode_vector_blob(digest_buffer);
		
		printf("encode/decode verify: %d\n", gsl_vector_equal(digest, digest2));
		printf("size verify:%d\n", get_vector_blob_size(digest_buffer));
		

		////////////////////////////////
		label_buffer = encode_vector(label);		
		label2 = decode_vector_blob(label_buffer);
		
		printf("encode/decode verify: %d\n", gsl_vector_equal(label, label2));
		printf("size verify:%d\n", get_vector_blob_size(label_buffer));
		*/		
}


int main(int argc, char* argv[]) {
	//UINT visit_count = 0;
	ULONG nodeid = 0;

	printf("SADS Prover\n");
	if(argc != 3)
	{
		printf("Usage: ./sads_prover <params_filename> <ip_addr>\n");
		exit(-1);
	}
	
	/** initialize() **/
	initialize(argv[1]);
	smysql_init();

	/** update() **/
	nodeid = get_nodeid_from_string(argv[2]);

	//update_leaf(nodeid);
	//update_path_labels(nodeid);

	//update_leaf(nodeid);
	//update_path_labels(nodeid);


	/** query() **/
	process_membership_query(nodeid);


	/** test **/

	return 0;
}













