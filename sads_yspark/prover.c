#include <stdio.h>
#include <math.h>
//#include <gsl/gsl_matrix.h>
//#include <gsl/gsl_blas.h>

#include "prover.h"
#include "sads_mysql.h"

/********************************************************
*
*	initialize()
*
*********************************************************/
void init_prover(char* filename) {
	read_params(filename);
	
	root_digest = get_initial_digest(FALSE);
}







/********************************************************
*
*	update()
*
*********************************************************/
void update_prover(ULONG nodeid) {
	update_leaf(nodeid);
	update_path_labels(nodeid);
}

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
		label = get_binary_representation(get_initial_digest(TRUE));
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

	//gsl_vector_fprintf(stdout, partial_label, "%f");

	update_node_label(nodeid, partial_label);

	return partial_label;
}


void update_node_label(ULONG nodeid, gsl_vector *partial_label) {
	gsl_vector *label = NULL;
	char *label_buffer = NULL;

	DEBUG_TRACE(("update_node_label(): nodeid(%llu)\n", nodeid));

	label = decode_vector_buffer(smysql_get_node_label(nodeid), LABEL_BUFFER_LEN);

	if(label == NULL) {
		label_buffer = encode_vector(partial_label);
		smysql_add_inter_node(nodeid, label_buffer, LABEL_BUFFER_LEN);
	}
	else {

		//printf("label size:%d, partial_label:%d\n", (UINT)label->size, (UINT)partial_label->size);

		gsl_vector_add(label, partial_label);
		mod_q(label, q);

		//gsl_vector_fprintf(stdout, label, "%f");

		label_buffer = encode_vector(label);

		smysql_update_inter_node(nodeid, label_buffer, LABEL_BUFFER_LEN);
	}
}








/********************************************************
*
*	query()
*
*********************************************************/
Proof *process_membership_query(ULONG nodeid) {
	Proof *proof;
	ULONG nodeid_list[(get_tree_height()-1)*2];
	UINT nodeid_num = 0;
	UINT i = 0;
	char *label_buffer = NULL;

	/** 1. get nodeids on the path.  Build proof->nodeid_num, proof->nodeid_list **/
	ULONG child_of_root_nodeid = nodeid >> (get_number_of_bits(nodeid)-2);

	DEBUG_TRACE(("process_membership_query(%llu)\n", nodeid));

	build_proof_path(child_of_root_nodeid, nodeid, &nodeid_num, nodeid_list);
	DEBUG_TRACE(("build_proof_path done (%llu, %llu, %u)\n", child_of_root_nodeid, nodeid, nodeid_num));

	proof = malloc(sizeof(Proof));
	proof->query_nodeid = nodeid;
	proof->answer = smysql_get_leaf_val(nodeid);
	//proof->nodeid_num = temp_nodeid_num;
	//proof->nodeid_list = malloc(sizeof(ULONG)*get_tree_height()*2);
	//memcpy(proof->nodeid_list, temp_nodeid_list, sizeof(ULONG)*get_tree_height()*2);


	/** 2. get labels of nodes included in proof->nodeid->list **/
	proof->label_list = malloc(nodeid_num * sizeof(char *));
	for(i=0; i < nodeid_num; i++) {
		proof->label_list[i] = malloc(LABEL_BUFFER_LEN);

		label_buffer = smysql_get_node_label(nodeid_list[i]);

		if(label_buffer != NULL) {
			//printf("valid label!!\n");

			memcpy(proof->label_list[i], label_buffer, LABEL_BUFFER_LEN);

			//gsl_vector_fprintf(stdout, decode_vector_buffer(proof->label_list[i], LABEL_BUFFER_LEN), "%f");
			//proof->label_list[i] = label_buffer;
		}
		else {
			//printf("null label!!\n");
			memset(proof->label_list[i], 0, LABEL_BUFFER_LEN);
		}
	}

	/** 3. return proof **/
	//printf("return proof\n");
	return proof;
}

void build_proof_path(ULONG child_of_root_nodeid, ULONG decendent_nodeid, UINT *temp_nodeid_num, ULONG *temp_nodeid_list) {
	UINT curr_nodeid_num = *temp_nodeid_num;

	DEBUG_TRACE(("build_proof_path(%llu, %llu, %u).\n", child_of_root_nodeid, decendent_nodeid, *temp_nodeid_num));

	if(child_of_root_nodeid > decendent_nodeid) {
		printf("Error build_proof_path(%llu, %llu)\n", child_of_root_nodeid, decendent_nodeid);
		exit(1);
	}

	/** Add decendent_nodeid and its sibling to the list **/
	temp_nodeid_list[curr_nodeid_num++] = decendent_nodeid;
	temp_nodeid_list[curr_nodeid_num++] = (decendent_nodeid ^ (ULONG)1);

	*temp_nodeid_num = curr_nodeid_num;

	if(child_of_root_nodeid < decendent_nodeid)
		build_proof_path(child_of_root_nodeid, decendent_nodeid >> 1, temp_nodeid_num, temp_nodeid_list);

#if 0
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
#endif
}


void write_proof(Proof *proof, UINT index) {
	char filename[20];
	FILE *fp;
	UINT i = 0;
	UINT label_num = (get_tree_height()-1) * 2;

	DEBUG_TRACE(("write_proof(%d)\n", index));

	sprintf(filename, "sads_proof_%d.dat", index);
	fp = fopen(filename, "wb");

	DEBUG_TRACE(("nodeid:%llu\n", proof->query_nodeid));
	fwrite(&(proof->query_nodeid), sizeof(ULONG), 1, fp);

	DEBUG_TRACE(("answer:%u\n", proof->answer));
	fwrite(&(proof->answer), sizeof(UINT), 1, fp);

	for(i=0; i<label_num; i++) {
#if 0
		printf("***proof->label_list[%d]\n", i);


		//gsl_vector_fprintf(stdout, decode_vector_buffer(proof->label_list[i], LABEL_BUFFER_LEN), "%f");

		if(gsl_vector_max(decode_vector_buffer(proof->label_list[i], LABEL_BUFFER_LEN)) > 0) {
			int j;
			for(j=0; j<LABEL_BUFFER_LEN; j++) {
				if(proof->label_list[i][j])
					printf("(%d,%d): %x\n", i, j, proof->label_list[i][j]);
			}
		}
#endif

		fwrite(proof->label_list[i], sizeof(char), LABEL_BUFFER_LEN, fp);
	}

	fclose(fp);
}

/********************************************************
*
*	misc.
*
*********************************************************/
#if 0
gsl_vector* get_initial_label(BOOL isLeaf) {
	if(isLeaf) {
		return get_binary_representation(get_initial_digest(TRUE));
	}
	else {

	}

}


ULONG get_nodeid(UINT ip_addr) {
	return ((ULONG)1 << 32) + (ULONG)ip_addr;
}



UINT get_label_buffer_len() {
	return LABEL_LEN * ELEMENT_LEN;
}

UINT get_digest_buffer_len() {
	return DIGEST_LEN * ELEMENT_LEN;
}
#endif








#if 0
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
#endif

/********************************************************
*
*	temporary test functions
*
*********************************************************/
void run_test(char* node_input_filename) {
	UINT num_nodes = 0;
	UINT i = 0;
	ULONG *node_list = read_node_input(node_input_filename, &num_nodes);
	Proof *proof;

	DEBUG_TRACE(("Benchmark Test: num_nodes(%u)", num_nodes));

	/** Prover Benchmark **/
	/** update() **/
#if 0
	for(i=0; i<num_nodes; i++) {
		update_prover(node_list[i]);
	}
#endif

	/** query() **/
	for(i=0; i<num_nodes; i++) {
		proof = process_membership_query(node_list[i]);



		write_proof(proof, i);
	}

}

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

	//Proof *proof = NULL;

	printf("SADS Prover\n");
	if(argc != 3)
	{
		printf("Usage: ./sads_prover <params_filename> <nodes_input_filename>\n");
		exit(-1);
	}
	
	/** initialize() **/
	init_prover(argv[1]);
	smysql_init();

	//nodeid = get_nodeid_from_string(argv[2]);


	/** update() **/
	//update_prover(nodeid);


	/** query() **/
	//proof = process_membership_query(nodeid);
	//write_proof(proof);


	/** test **/
	run_test(argv[2]);

	return 0;
}













