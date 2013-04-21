#include "verifier.h"

/********************************************************
*
*	initialize()
*
*********************************************************/
void init_verifier(char* param_filename) {
	read_params(param_filename);

	root_digest = get_initial_digest(FALSE);
}

/********************************************************
*
*	update()
*
*********************************************************/
void update_verifier(ULONG nodeid) {
	DEBUG_TRACE(("update_verifier: (%llu)", nodeid));

	update_root_digest(get_partial_digest((UINT)1, nodeid));
}


gsl_vector *get_partial_digest(ULONG nodeid, ULONG wrt_nodeid) {
	gsl_vector *partial_digest = NULL;
	int bit_shift_count =  get_number_of_bits(wrt_nodeid) - get_number_of_bits(nodeid);

	DEBUG_TRACE(("update_partial_digest(): nodeid(%llu), wrt_nodeid(%llu), bit_shift(%d)\n", nodeid, wrt_nodeid, bit_shift_count));

	if(bit_shift_count < 0) {
		printf("Error update_partial_digest(%llu: %llu)\n", nodeid, wrt_nodeid);
		exit(1);
	}
	/* reaches a leaf node */
	else if(bit_shift_count == 0) {
		if(nodeid != wrt_nodeid) {
			printf("Error nodeid(%llu) == wrt_nodeid(%llu)", nodeid, wrt_nodeid);
			exit(1);
		}
		DEBUG_TRACE(("Reached a leaf node\n"));
		return get_initial_digest(TRUE);
	}

	partial_digest = gsl_vector_alloc(DIGEST_LEN);
	//printf("partial label: %u\n", (UINT)partial_label->size);

	/* Right child */
	if(wrt_nodeid & ((ULONG)1 << (bit_shift_count-1))) {
		gsl_blas_dgemv(CblasNoTrans,\
				1.0, \
				R, \
				get_binary_representation(get_partial_digest((nodeid<<1)+(ULONG)1, wrt_nodeid)), \
				0.0, \
				partial_digest);
	}
	/* Left child */
	else {
		gsl_blas_dgemv(CblasNoTrans,\
				1.0, \
				L, \
				get_binary_representation(get_partial_digest(nodeid<<1, wrt_nodeid)), \
				0.0, \
				partial_digest);
	}

	mod_q(partial_digest, q);

	return partial_digest;
}


void update_root_digest(gsl_vector *partial_digest) {
	gsl_vector_add(root_digest, partial_digest);
	mod_q(root_digest, q);
}



/********************************************************
*
*	verify()
*
*********************************************************/
BOOL verify_membership_proof(MembershipProof *proof) {
	UINT i = 0;
	ULONG nodeid = proof->query_nodeid;
	gsl_vector *y = NULL;
	gsl_vector *label_lchild = NULL, *label_rchild = NULL;
	gsl_vector *partial_digest_lchild = gsl_vector_alloc(DIGEST_LEN);
	gsl_vector *partial_digest_rchild = gsl_vector_alloc(DIGEST_LEN);

	DEBUG_TRACE(("verify proof: (%llu)\n", proof->query_nodeid));

	/** verify root digest **/
	DEBUG_TRACE(("verify root digest: (%llu)\n", proof->query_nodeid));

	if(nodeid & (1 << (UNIVERSE_SIZE_IN_BIT-2))) {
		label_rchild =  decode_vector_buffer(proof->label_list[(get_tree_height()-1) * 2 - 2], LABEL_BUFFER_LEN);
		label_lchild =  decode_vector_buffer(proof->label_list[(get_tree_height()-1) * 2 - 1], LABEL_BUFFER_LEN);
	}
	else {
		label_lchild =  decode_vector_buffer(proof->label_list[(get_tree_height()-1) * 2 - 2], LABEL_BUFFER_LEN);
		label_rchild =  decode_vector_buffer(proof->label_list[(get_tree_height()-1) * 2 - 1], LABEL_BUFFER_LEN);
	}

	gsl_blas_dgemv(CblasNoTrans,\
			1.0, \
			L, \
			label_lchild, \
			0.0, \
			partial_digest_lchild);

	gsl_blas_dgemv(CblasNoTrans,\
			1.0, \
			R, \
			label_rchild, \
			0.0, \
			partial_digest_rchild);

	gsl_vector_add(partial_digest_lchild, partial_digest_rchild);

	y = partial_digest_lchild;
	mod_q(y, q);

	//gsl_vector_fprintf(stdout, y, "%f");

	if(!gsl_vector_equal(y, root_digest)) {
		DEBUG_TRACE(("Verification failed: root_digest\n"));
		return FALSE;
	}


	/** verify whole proof **/
	for(i=0; i<(get_tree_height()-1) * 2; i+=2) {
		if(i == 0) {
			y = get_initial_digest(TRUE);

			DEBUG_TRACE(("verify leaf label\n"));

			gsl_vector_scale(y, (double)proof->answer);

			if(!verify_radix(y, decode_vector_buffer(proof->label_list[i], LABEL_BUFFER_LEN))) {
					DEBUG_TRACE(("Verification failed: leaf(%d)\n", i));
					return FALSE;
			}
		}
		else {

			DEBUG_TRACE(("verify intermediate label(%u)\n", i));

			if(nodeid & (1 << (i/2-1))) {
				label_rchild = decode_vector_buffer(proof->label_list[i-2], LABEL_BUFFER_LEN);
				label_lchild = decode_vector_buffer(proof->label_list[i-1], LABEL_BUFFER_LEN);
			}
			else {
				label_lchild = decode_vector_buffer(proof->label_list[i-2], LABEL_BUFFER_LEN);
				label_rchild = decode_vector_buffer(proof->label_list[i-1], LABEL_BUFFER_LEN);
			}

			/*
			printf("L: %d, %d\n", L->size1, L->size2);
			printf("R: %d, %d\n", R->size1, R->size2);
			printf("label_lchild: %d\n", label_lchild->size);
			printf("label_rchild: %d\n", label_rchild->size);
			printf("left_partial_digest: %d\n", left_partial_digest->size);
			printf("right_partial_digest: %d\n", right_partial_digest->size);

			printf("label_lchild\n");
			gsl_vector_fprintf(stdout, label_lchild, "%f");
			printf("label_rchild\n");
			gsl_vector_fprintf(stdout, label_lchild, "%f");
			*/

			gsl_blas_dgemv(CblasNoTrans,\
					1.0, \
					L, \
					label_lchild, \
					0.0, \
					partial_digest_lchild);

			gsl_blas_dgemv(CblasNoTrans,\
					1.0, \
					R, \
					label_rchild, \
					0.0, \
					partial_digest_rchild);

			gsl_vector_add(partial_digest_lchild, partial_digest_rchild);

			y = partial_digest_lchild;
			mod_q(y, q);

			//gsl_vector_fprintf(stdout, y, "%f");

			if(!verify_radix(y, decode_vector_buffer(proof->label_list[i], LABEL_BUFFER_LEN))) {
				DEBUG_TRACE(("Verification failed(%d)\n", i));
				return FALSE;
			}


		}

	}

	return TRUE;
}



BOOL verify_range_proof(RangeProof *proof) {
	GList *leaf_nodeid_list = NULL;
	GHashTable *nodeid_table = NULL;

	UINT i = 0;

	for(i=0; i<proof->num_nodeid; i++) {
		if(proof->nodeid_list[i] & (ULONG)1 << 32) {
			printf("leaf found: %llu\n", proof->nodeid_list[i]);
			leaf_nodeid_list = g_list_append(leaf_nodeid_list, &(proof->nodeid_list[i]));
		}
	}

	while(leaf_nodeid_list) {
		printf("%llu\n", *(ULONG *)(leaf_nodeid_list->data));
		leaf_nodeid_list = leaf_nodeid_list->next;
	}


	return TRUE;

}




BOOL verify_radix_leaf(UINT y_leaf, gsl_vector *label) {
	DEBUG_TRACE(("Proof verify_radix_leaf: not implemented\n"));

	return TRUE;
}


BOOL verify_radix(gsl_vector *y, gsl_vector *label) {
	UINT i = 0, j = 0;
	UINT reverse_radix = 0;

	// length check
	if(y->size * log_q_ceil != label->size) {
		DEBUG_TRACE(("Proof verify_radix failed: Wrong lengths(%u, %u)\n", (UINT)y->size, (UINT)label->size));
		return FALSE;
	}

	for(i = 0; i < y->size; i++) {
		reverse_radix = 0;
		for(j = 0; j < log_q_ceil; j++) {
			//printf("(%u, %d):%f\n", i, j, label->data[i*log_q_ceil + j]);
			reverse_radix += (UINT)(label->data[i*log_q_ceil + j]) << (log_q_ceil - j - 1);
		}

		reverse_radix = reverse_radix % q;

		//printf("reverse_radix:%u, digest:%u\n", reverse_radix, (UINT)y->data[i]);

		if(reverse_radix != (UINT)y->data[i]) {
			DEBUG_TRACE(("Proof verify_radix failed: Wrong label or digest (%u), (%u, %u)\n", i, reverse_radix, (UINT)y->data[i]));

			printf("y\n");
			gsl_vector_fprintf(stdout, y, "%f");

			printf("label\n");
			gsl_vector_fprintf(stdout, label, "%f");


			return FALSE;
		}
	}

	//DEBUG_TRACE(("Proof verify_radix pass\n"));

	return TRUE;
}



/********************************************************
*
*	misc()
*
*********************************************************/


/********************************************************
*
*	Benchmark
*
*********************************************************/
void run_test(char* node_input_filename) {
	UINT num_nodes = 0;
	UINT i = 0;
	ULONG *node_list = read_node_input(node_input_filename, &num_nodes);
	MembershipProof *membership_proof;
	RangeProof *range_proof;

	DEBUG_TRACE(("Benchmark Test: num_nodes(%u)", num_nodes));

	/** Verifier Benchmark **/
	/** update() **/
	for(i=0; i<num_nodes; i++) {
		update_verifier(node_list[i]);
	}

	/** verify() **/
#if 0
	for(i=0; i<num_nodes; i++) {
		membership_proof = read_membership_proof(i);

		if(!verify_membership_proof(proof)) {
			DEBUG_TRACE(("Membership proof: Verification failed(%d)\n", i));
		}
	}
#endif

	range_proof = read_range_proof(0);

	if(!verify_range_proof(range_proof)) {
		DEBUG_TRACE(("Range proof: Verification failed(%d)\n", 0));
	}


}

MembershipProof *read_membership_proof(UINT index) {
	char filename[20];
	FILE *fp;
	UINT i = 0;
	UINT label_num = (get_tree_height()-1) * 2;
	MembershipProof *proof = NULL;

	DEBUG_TRACE(("read_proof(%d)\n", index));

	sprintf(filename, "sads_proof_%d.dat", index);
	fp = fopen(filename, "rb");

	if(fp == NULL) {
		printf("Proof file does not exist: %s\n", filename);
		exit(-1);
	}

	proof = malloc(sizeof(MembershipProof));
	fread(&proof->query_nodeid, sizeof(ULONG), 1, fp);
	fread(&proof->answer, sizeof(UINT), 1, fp);

	DEBUG_TRACE(("proof->query_nodeid(%llu)\n", proof->query_nodeid));
	DEBUG_TRACE(("proof->answer(%u)\n", proof->answer));


	proof->label_list = malloc(((get_tree_height()-1) * 2) * sizeof(char *));

	for(i=0; i<label_num; i++) {
		//int j;

		proof->label_list[i] = malloc(LABEL_BUFFER_LEN);
		fread(proof->label_list[i], ELEMENT_LEN, LABEL_LEN, fp);


#if 0
		printf("read_proof(%d, %d)\n", index, i);


		for(j=0; j<LABEL_BUFFER_LEN; j++) {
			//if(proof->label_list[i][j] != 0)
				printf("(%u, %u):%x\n", i, j, (char)proof->label_list[i][j]);
		}


		//gsl_vector_fprintf(stdout, decode_vector_buffer(proof->label_list[i], LABEL_BUFFER_LEN), "%f");
		//printf("max:%f\n", gsl_vector_max(decode_vector_buffer(proof->label_list[i], LABEL_BUFFER_LEN)));
#endif
	}

	fclose(fp);

	return proof;
}


RangeProof *read_range_proof(UINT index) {
	char filename[20];
	FILE *fp;
	UINT i = 0;
	//UINT label_num = 0;

	RangeProof *proof = NULL;
	UINT num_node = 0;


	DEBUG_TRACE(("read_rangeproof(%d)\n", index));

	sprintf(filename, "sads_range_proof.dat");
	fp = fopen(filename, "rb");

	if(fp == NULL) {
		printf("Proof file does not exist: %s\n", filename);
		exit(-1);
	}

	proof = malloc(sizeof(RangeProof));

	/** start_nodeid, end_nodeid, num_nodeid **/
	fread(&proof->start_nodeid, sizeof(ULONG), 1, fp);
	fread(&proof->end_nodeid, sizeof(ULONG), 1, fp);
	fread(&proof->num_nodeid, sizeof(UINT), 1, fp);

	DEBUG_TRACE(("range: %llu , %llu, %u\n", proof->start_nodeid, proof->end_nodeid, proof->num_nodeid));

	num_node = proof->num_nodeid;

	/** nodeid_list **/
	proof->nodeid_list = malloc(sizeof(ULONG) * num_node);
	fread(proof->nodeid_list, sizeof(ULONG), num_node, fp);

	/** answer_list **/
	proof->answer_list = malloc(sizeof(UINT) * num_node);
	fread(proof->answer_list, sizeof(UINT), num_node, fp);

	/** label_buffer_list **/
	proof->label_buffer_list = malloc(sizeof(char *) * num_node);

	for(i=0; i<num_node; i++) {
		proof->label_buffer_list[i] = malloc(LABEL_BUFFER_LEN);
		fread(proof->label_buffer_list[i], LABEL_BUFFER_LEN, 1, fp);
	}

	fclose(fp);

	return proof;
}

/********************************************************
*
*	main()
*
*********************************************************/
int main(int argc, char* argv[]) {
	//ULONG nodeid = 0;

	printf("SADS Verifier\n");
	if(argc != 3)
	{
		printf("Usage: ./verifier <params_filename> <nodes_input_filename>\n");
		exit(-1);
	}

	/** initialize() **/
	init_verifier(argv[1]);

	/** update **/
	//update_verifier(nodeid);

	/** query **/

	/** benchmark **/
	run_test(argv[2]);


	return 0;
}
