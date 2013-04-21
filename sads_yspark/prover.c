#include <stdio.h>
#include <math.h>

#include <glib.h>

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

	curr_count = smysql_get_leaf_val(nodeid);

	/** Add a new leaf node */
	if(curr_count == 0) {
		DEBUG_TRACE(("update_leaf: add a new leaf(%llu)\n", nodeid));

		smysql_add_node(nodeid, encode_vector(get_initial_label(TRUE)), (UINT)LABEL_BUFFER_LEN, TRUE);
	}
	/** Update an existing leaf node */
	else {
		DEBUG_TRACE(("update_leaf: update a leaf(%llu)\n", nodeid));

		gsl_vector *label = decode_vector_buffer(smysql_get_node_label(nodeid), LABEL_BUFFER_LEN);
		gsl_vector *init_label = get_initial_label(TRUE);

		if(label) {
			gsl_vector_add(label, get_initial_label(TRUE));
		}
		else {
			DEBUG_TRACE(("IT SHOULD BE UNREACHABLE\n"));
			label = init_label;
		}

		smysql_update_node(nodeid, curr_count+1, encode_vector(label), (UINT)LABEL_BUFFER_LEN, TRUE);
	}

	return (curr_count+1);
}


void update_path_labels(ULONG nodeid) {
	ULONG child_of_root_nodeid = nodeid >> (get_number_of_bits(nodeid)-2);

	DEBUG_TRACE(("update_path_labels(%llu, %llu)\n", child_of_root_nodeid, nodeid));

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

		return label;
	}

	partial_digest = gsl_vector_alloc(DIGEST_LEN);
	partial_label = gsl_vector_alloc(LABEL_LEN);

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

	mod_q(partial_digest, q);

	partial_label = get_binary_representation(partial_digest);

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
		smysql_add_node(nodeid, label_buffer, LABEL_BUFFER_LEN, FALSE);
	}
	else {

		gsl_vector_add(label, partial_label);
		mod_q(label, q);

		label_buffer = encode_vector(label);

		smysql_update_node(nodeid, 0, label_buffer, LABEL_BUFFER_LEN, FALSE);
	}


#if 0
	/** self verification **/
	if(label == NULL)
		label = partial_label;
	verify_label(nodeid, label);
#endif
}



void verify_label(ULONG nodeid, gsl_vector *label) {

	gsl_vector *rchild=NULL, *lchild=NULL;
	gsl_vector *v1_digest=NULL, *v2_digest = NULL;
	gsl_vector *rchild_partial_digest = NULL, *lchild_partial_digest = NULL;

	lchild = decode_vector_buffer(smysql_get_node_label(nodeid << 1), LABEL_BUFFER_LEN);
	rchild = decode_vector_buffer(smysql_get_node_label((nodeid << 1) + (ULONG)1), LABEL_BUFFER_LEN);

	//V2
	lchild_partial_digest = gsl_vector_alloc(DIGEST_LEN);
	rchild_partial_digest = gsl_vector_alloc(DIGEST_LEN);

	if(lchild == NULL) {
		lchild = gsl_vector_alloc(LABEL_LEN);
		gsl_vector_set_zero(lchild);
	}
	if(rchild == NULL) {
		rchild = gsl_vector_alloc(LABEL_LEN);
		gsl_vector_set_zero(rchild);
	}

	gsl_blas_dgemv(CblasNoTrans,\
			1.0, \
			L, \
			lchild, \
			0.0, \
			lchild_partial_digest);

	gsl_blas_dgemv(CblasNoTrans,\
			1.0, \
			R, \
			rchild, \
			0.0, \
			rchild_partial_digest);

	gsl_vector_add(lchild_partial_digest, rchild_partial_digest);
	v2_digest = lchild_partial_digest;
	mod_q(v2_digest, q);


	//V1
	v1_digest = get_digest_from_label(label);
	mod_q(v1_digest, q);


	// compare
	if(!gsl_vector_equal(v1_digest, v2_digest)) {
		printf("Self verification failed (%llu)\n", nodeid);
		exit(-1);
	}

}


gsl_vector *get_digest_from_label(gsl_vector *label) {
	gsl_vector *digest = gsl_vector_alloc(DIGEST_LEN);
	UINT i=0, j=0;
	UINT digest_value;

	for(i=0; i<LABEL_LEN; i++) {
		if(i % log_q_ceil == 0)
			digest_value = 0;

		digest_value += (UINT)(label->data[i]) << (log_q_ceil - 1 - (i % log_q_ceil) );

		if(i % log_q_ceil == (log_q_ceil - 1))
			digest->data[j++] = digest_value;
	}

	return digest;
}




/********************************************************
*
*	query()
*
*********************************************************/
MembershipProof *process_membership_query(ULONG nodeid) {
	MembershipProof *proof;
	ULONG nodeid_list[(get_tree_height()-1)*2];
	UINT nodeid_num = (get_tree_height()-1)*2;
	UINT i = 0;
	char *label_buffer = NULL;

	/** 1. get nodeids on the path.  Build proof->nodeid_num, proof->nodeid_list **/
	ULONG child_of_root_nodeid = nodeid >> (get_number_of_bits(nodeid)-2);

	DEBUG_TRACE(("process_membership_query(%llu)\n", nodeid));

	build_membership_proof_path(child_of_root_nodeid, nodeid, nodeid_num, nodeid_list);
	DEBUG_TRACE(("build_membership_proof_path done (%llu, %llu, %u)\n", child_of_root_nodeid, nodeid, nodeid_num));

	proof = malloc(sizeof(MembershipProof));
	proof->query_nodeid = nodeid;
	proof->answer = smysql_get_leaf_val(nodeid);

	/** 2. get labels of nodes included in proof->nodeid->list **/
	proof->label_list = malloc(nodeid_num * sizeof(char *));
	for(i=0; i < nodeid_num; i++) {
		proof->label_list[i] = malloc(LABEL_BUFFER_LEN);

		label_buffer = smysql_get_node_label(nodeid_list[i]);

		if(label_buffer != NULL) {
			memcpy(proof->label_list[i], label_buffer, LABEL_BUFFER_LEN);
		}
		else {
			memset(proof->label_list[i], 0, LABEL_BUFFER_LEN);
		}
	}

	/** 3. return proof **/
	return proof;
}

void build_membership_proof_path(ULONG child_of_root_nodeid, ULONG leaf_nodeid, UINT nodeid_num, ULONG *nodeid_list) {
	UINT i = 0;
	ULONG curr_nodeid = leaf_nodeid;

	DEBUG_TRACE(("build_proof_path(%llu, %llu, %u).\n", child_of_root_nodeid, leaf_nodeid, nodeid_num));

	for(i=0; i < nodeid_num; i+=2) {
		if(child_of_root_nodeid > curr_nodeid) {
			printf("Error build_proof_path(%llu, %llu): (%u, %llu)\n", child_of_root_nodeid, leaf_nodeid, i, curr_nodeid);
			exit(1);
		}

		nodeid_list[i] = curr_nodeid;
		nodeid_list[i+1] = (curr_nodeid ^ (ULONG)1);

		curr_nodeid = curr_nodeid >> 1;

	}
}


void write_membership_proof(MembershipProof *proof, UINT index) {
	char filename[20];
	FILE *fp;
	UINT i = 0;
	UINT label_num = (get_tree_height()-1) * 2;

	DEBUG_TRACE(("write_membership_proof(%d)\n", index));

	sprintf(filename, "sads_membership_proof_%d.dat", index);
	fp = fopen(filename, "wb");

	DEBUG_TRACE(("nodeid:%llu\n", proof->query_nodeid));
	fwrite(&(proof->query_nodeid), sizeof(ULONG), 1, fp);

	DEBUG_TRACE(("answer:%u\n", proof->answer));
	fwrite(&(proof->answer), sizeof(UINT), 1, fp);

	for(i=0; i<label_num; i++) {
		fwrite(proof->label_list[i], ELEMENT_LEN, LABEL_LEN, fp);
	}

	fclose(fp);
}



RangeProof *process_range_query(ULONG start_nodeid, ULONG end_nodeid) {
	//UINT num_nodeid = 0;
	//ULONG *nodeid_list = 0;
	//UINT *value_list = 0;
	//char **label_buffer_list = NULL;
	RangeProof *proof = malloc(sizeof(RangeProof));
	UINT num_answer_node = 0, num_proof_node = 0;

	GList *answer_nodeid_list = NULL;
	GHashTable *nodeid_table  = g_hash_table_new(g_int64_hash, g_int64_equal);


	DEBUG_TRACE(("process_range_query(%llu, %llu) init\n", start_nodeid, end_nodeid));

	/** 1. get nodeids of leaf nodes in the given range and put into hash table **/
	smysql_get_leaf_nodeids_in_range(start_nodeid, end_nodeid, nodeid_table);
	answer_nodeid_list = g_hash_table_get_keys(nodeid_table);


	/** 2. get nodeids of all relevant intermediate nodes in the tree. **/
	get_inter_nodeids_in_range(nodeid_table, answer_nodeid_list);


	/** 3. build proof of the range query **/
	// start, end nodeid
	proof->start_nodeid = start_nodeid;
	proof->end_nodeid = end_nodeid;

	// answer
	num_answer_node = g_list_length(answer_nodeid_list);
	proof->num_answer_nodeid = num_answer_node;
	proof->answer_nodeid_list = malloc(sizeof(UINT) * num_answer_node);
	proof->answer_list = malloc(sizeof(UINT) * num_answer_node);

	build_range_answer(proof, answer_nodeid_list);

	// proof
	num_proof_node = g_hash_table_size(nodeid_table);
	proof->num_proof_nodeid = num_proof_node;
	proof->proof_nodeid_list = malloc(sizeof(ULONG) * num_proof_node);
	proof->proof_label_buffer_list = malloc(sizeof(char *) * num_proof_node);

	build_range_proof(proof, nodeid_table);

	DEBUG_TRACE(("process_range_query done (num_answer_node:%u, num_proof_node:%u)\n", num_answer_node, num_proof_node));

	/** 4. return proof **/
	g_hash_table_destroy(nodeid_table);

	return proof;
}

void get_inter_nodeids_in_range(GHashTable *nodeid_table, GList *answer_nodeid_list) {

	DEBUG_TRACE(("get_inter_nodeids_in_range()\n"));

	if( answer_nodeid_list == NULL ){
		printf("Error get_inter_nodeids_in_range()\n");
		exit(1);
	}

	while(answer_nodeid_list) {
		ULONG curr_nodeid = *(ULONG *)(answer_nodeid_list->data);
		printf("%llu\n", curr_nodeid);
		printf("%d\n", g_list_length(answer_nodeid_list));

		/** Add nodes on the path to the root **/
		while(curr_nodeid > 1) {
			// add nodeid
			g_hash_table_insert (nodeid_table,
								g_memdup(&curr_nodeid, sizeof(curr_nodeid)),
								g_memdup(&curr_nodeid, sizeof(curr_nodeid)));

			// add sibling nodeid
			curr_nodeid = (curr_nodeid ^ (ULONG)1);
			g_hash_table_insert (nodeid_table,
								g_memdup(&curr_nodeid, sizeof(curr_nodeid)),
								g_memdup(&curr_nodeid, sizeof(curr_nodeid)));

			printf("(%llu, %llu, %llu) %d\n", curr_nodeid, curr_nodeid ^ (ULONG)1, *(ULONG *)(answer_nodeid_list->data), g_hash_table_size(nodeid_table));

			// go up to the parent node
			curr_nodeid = curr_nodeid >> 1;
		}

		answer_nodeid_list = answer_nodeid_list->next;
	}
}

void build_range_answer(RangeProof *proof, GList *answer_nodeid_list) {
	UINT node_count = 0;
	ULONG curr_nodeid = 0;

	DEBUG_TRACE(("build_range_answer()\n"));

	while(answer_nodeid_list) {
		curr_nodeid = *(ULONG *)(answer_nodeid_list->data);
		proof->answer_nodeid_list[node_count] = curr_nodeid;
		proof->answer_list[node_count] = smysql_get_leaf_val(curr_nodeid);

		printf("%llu, %u\n", proof->answer_nodeid_list[node_count], proof->answer_list[node_count]);

		node_count++;
		answer_nodeid_list = answer_nodeid_list->next;
	}
}


void build_range_proof(RangeProof *proof, GHashTable *nodeid_table) {
	GList *nodeid_list = g_hash_table_get_keys(nodeid_table);
	UINT node_count = 0;
	ULONG curr_nodeid = 0;
	UINT temp_answer = 0;

	DEBUG_TRACE(("build_range_proof()\n"));

	while(nodeid_list) {
		curr_nodeid = *(ULONG *)(nodeid_list->data);
		//memcpy(&proof->nodeid_list[node_count], &curr_nodeid, sizeof(ULONG));
		proof->proof_nodeid_list[node_count] = curr_nodeid;

		proof->proof_label_buffer_list[node_count] = malloc(LABEL_BUFFER_LEN);

		smysql_get_node_info(curr_nodeid, &temp_answer, proof->proof_label_buffer_list[node_count]);

		//printf("%llu, %u\n", proof->nodeid_list[node_count], proof->answer_list[node_count]);

		node_count++;
		nodeid_list = nodeid_list->next;

	}

}


#if 0
//UINT build_range_proof_path(ULONG start_nodeid, ULONG end_nodeid, ULONG *nodeid_list) {
void build_range_proof_path(RangeProof *proof) {
	UINT num_nodeid = 0;

	num_nodeid = smysql_get_range_result(proof->start_nodeid, proof->end_nodeid, &(proof->nodeid_list), &(proof->answer_list), &(proof->label_buffer_list));
}
#endif


void write_range_proof(RangeProof *proof) {
	char filename[20];
	FILE *fp;
	UINT i = 0;

	DEBUG_TRACE(("write_range_proof(%llu, %llu), num_answer(%u), num_proof(%u)\n", \
			proof->start_nodeid, proof->end_nodeid, proof->num_answer_nodeid, proof->num_proof_nodeid));

	sprintf(filename, "sads_range_proof.dat");
	fp = fopen(filename, "wb");

	/* Write start_nodeid, end_nodeid */
	fwrite(&(proof->start_nodeid), sizeof(ULONG), 1, fp);
	fwrite(&(proof->end_nodeid), sizeof(ULONG), 1, fp);

	/* Write answer */
	fwrite(&(proof->num_answer_nodeid), sizeof(UINT), 1, fp);
	fwrite(proof->answer_nodeid_list, sizeof(ULONG), proof->num_answer_nodeid, fp);
	fwrite(proof->answer_list, sizeof(UINT), proof->num_answer_nodeid, fp);

	/* Write proof */
	fwrite(&(proof->num_proof_nodeid), sizeof(UINT), 1, fp);
	fwrite(proof->proof_nodeid_list, sizeof(ULONG), proof->num_proof_nodeid, fp);

	/* Write label_buffer_list  */
	//printf("write label_buffer_list\n");
	for(i=0; i<proof->num_proof_nodeid; i++) {
		fwrite(proof->proof_label_buffer_list[i], ELEMENT_LEN, LABEL_LEN, fp);
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





/********************************************************
*
*	test functions
*
*********************************************************/
void run_test(char* node_input_filename) {
	UINT num_nodes = 0;
	UINT i = 0;
	ULONG *node_list = read_node_input(node_input_filename, &num_nodes);
	MembershipProof *membership_proof;
	RangeProof *range_proof;

	DEBUG_TRACE(("Benchmark Test: num_nodes(%u)", num_nodes));

	/** Prover Benchmark **/
	/** update() **/
#if 1
	for(i=0; i<num_nodes; i++) {
		update_prover(node_list[i]);
	}
#endif

	/** query() **/
#if 0
	// Membership Proof
	for(i=0; i<num_nodes; i++) {
		membership_proof = process_membership_query(node_list[i]);
		write_membership_proof(membership_proof, i);
	}
#endif

#if 1
	if(num_nodes > 1) {
		// Range Proof
		range_proof = process_range_query(node_list[0], node_list[num_nodes-1]);
		write_range_proof(range_proof);
	}
#endif
}


#if 0
void testMatrix() {
	gsl_matrix *A = gsl_matrix_alloc(k, m);

	printf("testMatrix\n");
	printf("%d %d\n", (int)L->size1, (int)L->size2);
	printf("%d %d\n", (int)R->size1, (int)R->size2);
	
	gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, L, R, 0.0, A);
	printf("%d %d\n", (int)A->size1, (int)A->size2);
	
	gsl_matrix_fprintf(stdout, A, "%f");
}
#endif


#if 0
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
#endif


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













