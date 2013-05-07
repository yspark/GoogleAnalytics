#include "verifier.h"

/*****************************************************************************
*
*	initialize()
**
*****************************************************************************/
void init_verifier(char* param_filename) {
	read_params(param_filename);

	root_digest = get_initial_digest(FALSE);
}


/*****************************************************************************
*
*	update()
*
*****************************************************************************/
void update_verifier(ULONG nodeid) {
	gsl_vector *partial_digest = get_partial_digest((UINT)1, nodeid);

	DEBUG_TRACE(("update_verifier: (%llu)", nodeid));

	update_root_digest(partial_digest);

	gsl_vector_free(partial_digest);
}


gsl_vector *get_partial_digest(ULONG nodeid, ULONG wrt_nodeid) {
	gsl_vector *partial_digest = NULL;
	gsl_vector *child_partial_digest = NULL;
	gsl_vector *child_partial_label = NULL;
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
		child_partial_digest = get_partial_digest((nodeid<<1)+(ULONG)1, wrt_nodeid);
		child_partial_label = get_binary_representation(child_partial_digest);

		gsl_blas_dgemv(CblasNoTrans,\
				1.0, \
				R, \
				child_partial_label, \
				0.0, \
				partial_digest);
	}
	/* Left child */
	else {
		child_partial_digest = get_partial_digest(nodeid<<1, wrt_nodeid);
		child_partial_label = get_binary_representation(child_partial_digest);

		gsl_blas_dgemv(CblasNoTrans,\
				1.0, \
				L, \
				child_partial_label, \
				0.0, \
				partial_digest);
	}

	mod_q(partial_digest, q);


	/** free memory */
	gsl_vector_free(child_partial_digest);
	gsl_vector_free(child_partial_label);

	return partial_digest;
}


void update_root_digest(gsl_vector *partial_digest) {
	gsl_vector_add(root_digest, partial_digest);
	mod_q(root_digest, q);
}



/*****************************************************************************
*
*	membership verify()
*
*****************************************************************************/
BOOL verify_membership_proof(MembershipProof *proof) {
	UINT i = 0;
	ULONG nodeid = proof->query_nodeid;
	ULONG rchild_nodeid = 0, lchild_nodeid = 0;
	UINT nodeid_index = 0;
	UINT rchild_nodeid_index = 0, lchild_nodeid_index = 0;

	gsl_vector *label = NULL;

	gsl_vector *y = NULL;
	gsl_vector *label_rchild = NULL, *label_lchild = NULL;
	gsl_vector *partial_digest_lchild = gsl_vector_alloc(DIGEST_LEN);
	gsl_vector *partial_digest_rchild = gsl_vector_alloc(DIGEST_LEN);

	GHashTable *nodeid_table  = g_hash_table_new(g_int64_hash, g_int64_equal);

	DEBUG_TRACE(("verify proof: (%llu)\n", proof->query_nodeid));


	/** build proof_nodeid hash table */
	for(i=0; i<proof->num_proof_nodeid; i++) {
		g_hash_table_insert (nodeid_table,
							g_memdup(&proof->proof_nodeid_list[i], sizeof(ULONG)),
							g_memdup(&i, sizeof(UINT)));
	}

	/** verify leaf node */
	nodeid = proof->query_nodeid;
	nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &nodeid));

	DEBUG_TRACE(("verify leaf label (%llu:%u)\n", nodeid, nodeid_index));

	y = get_initial_digest(TRUE);
	gsl_vector_scale(y, (double)proof->answer);

	label = decode_vector_buffer(proof->proof_label_list[nodeid_index], LABEL_BUFFER_LEN);
	if(!verify_radix(y, label)) {
			DEBUG_TRACE(("Leaf node verification failed: nodeid(%llu, %u)\n", nodeid, nodeid_index));
			return FALSE;
	}

	/* memory free */
	gsl_vector_free(label);
	gsl_vector_free(y);


	/** verify root digest **/
	DEBUG_TRACE(("verify root digest: (%llu)\n", proof->query_nodeid));

	lchild_nodeid = 2;
	rchild_nodeid = 3;

	lchild_nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &lchild_nodeid));
	rchild_nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &rchild_nodeid));

	DEBUG_TRACE(("lchild(%llu, %u), rchild(%llu, %u)\n", lchild_nodeid, lchild_nodeid_index, rchild_nodeid, rchild_nodeid_index));

	label_rchild = decode_vector_buffer(proof->proof_label_list[rchild_nodeid_index], LABEL_BUFFER_LEN);
	label_lchild = decode_vector_buffer(proof->proof_label_list[lchild_nodeid_index], LABEL_BUFFER_LEN);

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

	if(!gsl_vector_equal(y, root_digest)) {
		DEBUG_TRACE(("Verification failed: root_digest\n"));

		printf("y\n");
		gsl_vector_fprintf(stdout, y, "%f");
		printf("root_digest\n");
		gsl_vector_fprintf(stdout, root_digest, "%f");

		return FALSE;
	}

	/** free memory */
	gsl_vector_free(label_rchild);
	gsl_vector_free(label_lchild);
	//gsl_vector_free(partial_digest_rchild);
	//gsl_vector_free(partial_digest_lchild);
	//gsl_vector_free(y);


	/** Verify intermediate node labels */
	nodeid = nodeid >> 1;

	while(nodeid > 1) {
		nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &nodeid));

		DEBUG_TRACE(("verify intermediate label (%llu:%u)\n", nodeid, nodeid_index));

		rchild_nodeid = (nodeid << 1) + 1;
		lchild_nodeid = nodeid << 1;

		rchild_nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &rchild_nodeid));
		lchild_nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &lchild_nodeid));

		label_rchild = decode_vector_buffer(proof->proof_label_list[rchild_nodeid_index], LABEL_BUFFER_LEN);
		label_lchild = decode_vector_buffer(proof->proof_label_list[lchild_nodeid_index], LABEL_BUFFER_LEN);

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

		/** verify label */
		label = decode_vector_buffer(proof->proof_label_list[nodeid_index], LABEL_BUFFER_LEN);
		if(!verify_radix(y, label)) {
			DEBUG_TRACE(("Verification failed(%llu, %u)\n", nodeid, nodeid_index));
			return FALSE;
		}

		nodeid = nodeid >> 1;

		/** memory free */
		gsl_vector_free(label_rchild);
		gsl_vector_free(label_lchild);
		gsl_vector_free(label);
	}

	/** memory free */
	gsl_vector_free(partial_digest_lchild);
	gsl_vector_free(partial_digest_rchild);
	g_hash_table_destroy(nodeid_table);

	return TRUE;
}


MembershipProof *read_membership_proof(UINT index) {
	char filename[40];
	FILE *fp;
	UINT i = 0;
	int retVal;
	MembershipProof *proof = NULL;

	DEBUG_TRACE(("read_proof(%d)\n", index));

	sprintf(filename, "./proof/sads_membership_proof_%d.dat", index);
	fp = fopen(filename, "rb");

	if(fp == NULL) {
		printf("Proof file does not exist: %s\n", filename);
		exit(-1);
	}

	proof = malloc(sizeof(MembershipProof));

	/* query_nodeid, answer, num_proof_nodeid */
	retVal = fread(&proof->query_nodeid, sizeof(ULONG), 1, fp);
	check_retVal(retVal);
	retVal = fread(&proof->answer, sizeof(UINT), 1, fp);
	check_retVal(retVal);
	retVal = fread(&proof->num_proof_nodeid, sizeof(UINT), 1, fp);
	check_retVal(retVal);

	DEBUG_TRACE(("proof->query_nodeid(%llu)\n", proof->query_nodeid));
	DEBUG_TRACE(("proof->answer(%u)\n", proof->answer));
	DEBUG_TRACE(("proof->num_proof_nodeid(%u)\n", proof->num_proof_nodeid));

	/* nodeid_list */
	proof->proof_nodeid_list = malloc(sizeof(ULONG) * proof->num_proof_nodeid);
	retVal = fread(proof->proof_nodeid_list, sizeof(ULONG), proof->num_proof_nodeid, fp);
	check_retVal(retVal);

	/* label_list */
	proof->proof_label_list = malloc(sizeof(char *) * proof->num_proof_nodeid);

	for(i=0; i<proof->num_proof_nodeid; i++) {
		proof->proof_label_list[i] = malloc(LABEL_BUFFER_LEN);
		retVal = fread(proof->proof_label_list[i], ELEMENT_LEN, LABEL_LEN, fp);
		check_retVal(retVal);
	}

	fclose(fp);

	return proof;
}


/*****************************************************************************
*
*	range verify()
*
*****************************************************************************/
BOOL verify_range_proof(RangeProof *proof) {
	GHashTable *nodeid_table  = g_hash_table_new(g_int64_hash, g_int64_equal);

	UINT i = 0;
	ULONG nodeid = 0;
	ULONG rchild_nodeid = 0, lchild_nodeid = 0;
	UINT nodeid_index = 0;
	UINT rchild_nodeid_index = 0, lchild_nodeid_index = 0;

	gsl_vector *y = NULL;
	gsl_vector *label_rchild = NULL, *label_lchild = NULL;
	gsl_vector *partial_digest_lchild = gsl_vector_alloc(DIGEST_LEN);
	gsl_vector *partial_digest_rchild = gsl_vector_alloc(DIGEST_LEN);


	DEBUG_TRACE(("verify_range_proof(%u, %u)\n", proof->num_answer_nodeid, proof->num_proof_nodeid));


	/** build proof_nodeid hash table */
	for(i=0; i<proof->num_proof_nodeid; i++) {
		g_hash_table_insert (nodeid_table,
							g_memdup(&proof->proof_nodeid_list[i], sizeof(ULONG)),
							g_memdup(&i, sizeof(UINT)));
	}


	/** verify each answer nodeid */
	for(i=0; i<proof->num_answer_nodeid; i++) {
		nodeid = proof->answer_nodeid_list[i];
		nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &nodeid));

		/** Verify leaf node label */
		DEBUG_TRACE(("verify leaf label (%llu:%u)\n", nodeid, nodeid_index));

		y = get_initial_digest(TRUE);
		gsl_vector_scale(y, (double)proof->answer_list[i]);

		if(!verify_radix(y, decode_vector_buffer(proof->proof_label_list[nodeid_index], LABEL_BUFFER_LEN))) {
				DEBUG_TRACE(("Leaf node verification failed: nodeid(%llu, %u)\n", nodeid, nodeid_index));
				return FALSE;
		}

		/** Verify intermediate node labels */
		nodeid = nodeid >> 1;

		while(nodeid > 1) {
			nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &nodeid));

			DEBUG_TRACE(("verify intermediate label (%llu:%u)\n", nodeid, nodeid_index));

			rchild_nodeid = (nodeid << 1) + 1;
			lchild_nodeid = nodeid << 1;

			rchild_nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &rchild_nodeid));
			lchild_nodeid_index = *(UINT *)(g_hash_table_lookup(nodeid_table, &lchild_nodeid));

			label_rchild = decode_vector_buffer(proof->proof_label_list[rchild_nodeid_index], LABEL_BUFFER_LEN);
			label_lchild = decode_vector_buffer(proof->proof_label_list[lchild_nodeid_index], LABEL_BUFFER_LEN);

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


			if(!verify_radix(y, decode_vector_buffer(proof->proof_label_list[nodeid_index], LABEL_BUFFER_LEN))) {
				DEBUG_TRACE(("Verification failed(%llu, %u)\n", nodeid, nodeid_index));
				return FALSE;
			}

			nodeid = nodeid >> 1;
		}
	}

	return TRUE;

}



RangeProof *read_range_proof(UINT index) {
	char filename[40];
	FILE *fp;
	UINT i = 0;
	int retVal;

	RangeProof *proof = NULL;

	DEBUG_TRACE(("read_rangeproof(%d)\n", index));

	sprintf(filename, "./proof/sads_range_proof_%d.dat", index);
	fp = fopen(filename, "rb");

	if(fp == NULL) {
		printf("Proof file does not exist: %s\n", filename);
		return NULL;
		//exit(-1);
	}

	proof = malloc(sizeof(RangeProof));

	/** start_nodeid, end_nodeid **/
	retVal = fread(&proof->start_nodeid, sizeof(ULONG), 1, fp);
	check_retVal(retVal);
	retVal = fread(&proof->end_nodeid, sizeof(ULONG), 1, fp);
	check_retVal(retVal);

	/** answer */
	retVal = fread(&proof->num_answer_nodeid, sizeof(UINT), 1, fp);
	check_retVal(retVal);

	proof->answer_nodeid_list = malloc(sizeof(ULONG) * proof->num_answer_nodeid);
	retVal = fread(proof->answer_nodeid_list, sizeof(ULONG), proof->num_answer_nodeid, fp);
	check_retVal(retVal);

	proof->answer_list = malloc(sizeof(UINT) * proof->num_answer_nodeid);
	retVal = fread(proof->answer_list, sizeof(UINT), proof->num_answer_nodeid, fp);
	check_retVal(retVal);


	/** proof */
	retVal = fread(&proof->num_proof_nodeid, sizeof(UINT), 1, fp);
	check_retVal(retVal);

	proof->proof_nodeid_list = malloc(sizeof(ULONG) * proof->num_proof_nodeid);
	retVal = fread(proof->proof_nodeid_list, sizeof(ULONG), proof->num_proof_nodeid, fp);
	check_retVal(retVal);

	proof->proof_label_list = malloc(sizeof(char *) * proof->num_proof_nodeid);
	for(i=0; i<proof->num_proof_nodeid; i++) {
		proof->proof_label_list[i] = malloc(LABEL_BUFFER_LEN);
		retVal = fread(proof->proof_label_list[i], LABEL_BUFFER_LEN, 1, fp);
		check_retVal(retVal);
	}

	//DEBUG_TRACE(("rangg(%llu , %llu), num_nodes(%u, %d)\n", proof->start_nodeid, proof->end_nodeid, proof->answer_num_nodeid, proof->answer_num_nodeid));

	fclose(fp);

	return proof;
}



/********************************************************
*
*	misc()
*
*********************************************************/
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
*	Benchmark
*
*********************************************************/
void run_membership_test(char* node_input_filename, int num_query) {
	UINT num_nodes = 0;
	UINT i = 0;
	ULONG *node_list = read_node_input(node_input_filename, &num_nodes);
	MembershipProof *membership_proof;

	struct timeval  tv1, tv2;

	DEBUG_TRACE(("Benchmark Test(Membership): num_nodes(%u)", num_nodes));

	/** Verifier Benchmark **/
	/** update() **/
	printf("update start\n");
    gettimeofday(&tv1, NULL);

	for(i=0; i<num_nodes; i++) {
		update_verifier(node_list[i]);
		printf("%d/%d done.\n", i+1, num_nodes);
	}

	gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));

	/** verify() **/
	printf("membership verification start\n");
    gettimeofday(&tv1, NULL);

	for(i=0; i<num_query; i++) {
		membership_proof = read_membership_proof(i);

		if(!verify_membership_proof(membership_proof)) {
			//DEBUG_TRACE(("Membership proof: Verification failed(%d)\n", i));
			printf("%d/%d verification failed(%d)\n", i+1, num_query, i);
		}
		else {
			printf("%d/%d done.\n", i+1, num_query);
		}

		/** memory free */
		free_membership_proof(membership_proof);

	}

    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));


    /** memory free */
    free(node_list);
}


void run_range_test(char* node_input_filename, int num_query) {
	UINT num_nodes = 0;
	UINT i = 0;
	ULONG *node_list = read_node_input(node_input_filename, &num_nodes);
	RangeProof *range_proof;

    struct timeval  tv1, tv2;

	DEBUG_TRACE(("Benchmark Test(Range): num_nodes(%u)", num_nodes));

	/** Verifier Benchmark **/
	/** update() **/
	printf("update start\n");
    gettimeofday(&tv1, NULL);

	for(i=0; i<num_nodes; i++) {
		update_verifier(node_list[i]);
	}

	gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));


	/** verify() **/
	printf("range verification start\n");
    gettimeofday(&tv1, NULL);

	for(i=0; i<num_query; i++) {
		range_proof = read_range_proof(i);

		if(!verify_range_proof(range_proof)) {
			DEBUG_TRACE(("Range proof: Verification failed(%d)\n", 0));
		}
	}

    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));
}




/********************************************************
*
*	main()
*
*********************************************************/
int main(int argc, char* argv[]) {
	//ULONG nodeid = 0;

	printf("SADS Verifier\n");
	if(argc != 5)
	{
		printf("Usage: ./verifier <params_filename> <nodes_input_filename> <test_mode> <num_query>\n");
		exit(-1);
	}

	/** initialize() **/
	init_verifier(argv[1]);


	/** benchmark **/
	if(strcmp(argv[3], "membership") == 0) {
		run_membership_test(argv[2], atoi(argv[4]));
	}
	else if(strcmp(argv[3], "range") == 0) {
		run_range_test(argv[2], atoi(argv[4]));
	}
	else {
		printf("<test mode>: membership / range\n");
	}


	return 0;
}
