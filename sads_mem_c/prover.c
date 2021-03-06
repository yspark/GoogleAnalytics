#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include "prover.h"



/*****************************************************************************
*
*	initialize()
**
*****************************************************************************/
/**
 * This function reads in security parameters and build initial root digest.
 */
void init_prover(char* filename) {
	read_params(filename);
	
	root_digest = get_initial_digest(FALSE);

	/** key: nodeid, value: label vector */
	label_table = g_hash_table_new(g_int64_hash, g_int64_equal);

	/** key: nodeid, value: visiting counter */
	leaf_value_table = g_hash_table_new(g_int64_hash, g_int64_equal);
}





/*****************************************************************************
*
*	update()
*
*****************************************************************************/
/**
 * Given leaf nodeid, update the leaf node and nodes on the path to the root node
 */
void update_prover(ULONG nodeid) {
	//DEBUG_TRACE(("update_leaf result: (%llu, %u)\n", nodeid, update_leaf(nodeid)));
	update_leaf(nodeid);
	update_path_labels(nodeid);
}


/**
 * Add a new leaf node. Or update existing leaf node.
 */
UINT update_leaf(ULONG nodeid) {
	UINT *p_value = NULL;
	ULONG *p_nodeid = malloc(sizeof(ULONG));

	*p_nodeid = nodeid;
	p_value = (UINT *)g_hash_table_lookup(leaf_value_table, p_nodeid);

	gsl_vector *init_label = get_initial_label(TRUE);


	if(p_value == NULL) {
		DEBUG_TRACE(("update_leaf: add a new leaf(%llu)\n", nodeid));

		p_value = malloc(sizeof(UINT));
		*p_value = 1;

		g_hash_table_insert (leaf_value_table,
							p_nodeid,
							p_value);

		g_hash_table_insert (label_table,
							p_nodeid,
							init_label);

	}
	else {
		DEBUG_TRACE(("update_leaf: update a leaf(%llu)\n", nodeid));

		gsl_vector *label = g_hash_table_lookup(label_table, p_nodeid);
		gsl_vector_add(label, init_label);

		*p_value = *p_value + 1;
		g_hash_table_insert (leaf_value_table,
							p_nodeid,
							p_value);
		/*
		g_hash_table_insert (label_table,
							p_nodeid,
							label);
		*/
		if(init_label) gsl_vector_free(init_label);
	}



	return (*p_value);
}

/**
 * Update all the nodes on the path to the root node
 */
void update_path_labels(ULONG nodeid) {
	ULONG child_of_root_nodeid = nodeid >> (get_number_of_bits(nodeid)-2);

	DEBUG_TRACE(("update_path_labels(%llu, %llu)\n", child_of_root_nodeid, nodeid));

	update_partial_label(child_of_root_nodeid, nodeid);
}


/**
 * Auxilary function for update_path_labels(). Recursive function call.
 */
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
	//partial_label = gsl_vector_alloc(LABEL_LEN);

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

	/** free memory */
	gsl_vector_free(partial_digest);

	return partial_label;
}

/**
 * Given partial label, update the label
 */
void update_node_label(ULONG nodeid, gsl_vector *partial_label) {
	gsl_vector *label = NULL;
	ULONG *p_nodeid = malloc(sizeof(ULONG));
	*p_nodeid = nodeid;

	DEBUG_TRACE(("update_node_label(): nodeid(%llu)\n", nodeid));

	label = (gsl_vector *)g_hash_table_lookup(label_table, p_nodeid);

	if(label == NULL) {
		//printf("new label:\n");
		g_hash_table_insert(label_table, p_nodeid, partial_label);
	}
	else {
		//printf("updated label:\n");

		gsl_vector_add(label, partial_label);
		mod_q(label, q);

		//g_hash_table_insert(label_table, p_nodeid, label);

	}


	//gsl_vector_fprintf(stdout, g_hash_table_lookup(label_table, p_nodeid), "%f");
}



/*****************************************************************************
*
*	membership query()
*
*****************************************************************************/
MembershipProof *process_membership_query(ULONG nodeid) {
	MembershipProof *proof = malloc(sizeof(MembershipProof));;
	GList *proof_nodeid_list = NULL;
	UINT node_count = 0;
	UINT num_proof_node = 0;
	ULONG curr_nodeid = 0;
	char *label_buffer = NULL;

	/** 1. get nodeids of all relevant leaf/intermediate nodes in the tree. **/
	DEBUG_TRACE(("process_membership_query(%llu)\n", nodeid));

	proof_nodeid_list = build_membership_proof_path(nodeid);

	/** 3. build proof of the membership query **/
	// query_nodeid, answer
	proof->query_nodeid = nodeid;
	proof->answer = *(UINT *)g_hash_table_lookup(leaf_value_table, g_memdup(&nodeid, sizeof(nodeid)));

	//printf("answer:%u\n", proof->answer);


	// proof
	num_proof_node = g_list_length(proof_nodeid_list);
	proof->num_proof_nodeid = num_proof_node;
	proof->proof_nodeid_list = malloc(sizeof(ULONG) * num_proof_node);
	proof->proof_label_list = malloc(sizeof(char *) * num_proof_node);

	while(proof_nodeid_list) {
		curr_nodeid = *(ULONG *)(proof_nodeid_list->data);

		//printf("curr_nodeid:%llu\n", curr_nodeid);

		proof->proof_nodeid_list[node_count] = curr_nodeid;

		proof->proof_label_list[node_count] = malloc(LABEL_BUFFER_LEN);




		label_buffer = encode_vector((gsl_vector *)g_hash_table_lookup(label_table, g_memdup(&curr_nodeid, sizeof(curr_nodeid))));

		if(label_buffer) {
			//printf("label\n");
			memcpy(proof->proof_label_list[node_count], label_buffer, LABEL_BUFFER_LEN);
		}
		else {
			//printf("empty\n");
			memset(proof->proof_label_list[node_count], 0, LABEL_BUFFER_LEN);
		}

		free(label_buffer);

		node_count++;
		proof_nodeid_list = proof_nodeid_list->next;
	}

	/** 3. return proof **/
	g_list_free(proof_nodeid_list);

	return proof;
}


GList *build_membership_proof_path(ULONG leaf_nodeid) {
	GList *proof_nodeid_list = NULL;
	ULONG curr_nodeid = leaf_nodeid;

	DEBUG_TRACE(("build_proof_path(%llu).\n", leaf_nodeid));

	while(curr_nodeid > 1) {
		proof_nodeid_list = g_list_append(proof_nodeid_list, \
									g_memdup(&curr_nodeid, sizeof(curr_nodeid)));

		curr_nodeid = (curr_nodeid ^ (ULONG)1);
		proof_nodeid_list = g_list_append(proof_nodeid_list, \
									g_memdup(&curr_nodeid, sizeof(curr_nodeid)));

		curr_nodeid = curr_nodeid >> 1;
	}

	return proof_nodeid_list;
}


void write_membership_proof(MembershipProof *proof, UINT index) {
	char filename[40];
	FILE *fp;
	UINT i = 0;

	DEBUG_TRACE(("write_membership_proof(%d)\n", index));

	sprintf(filename, "./proof/sads_membership_proof_%d.dat", index);
	fp = fopen(filename, "wb");

	// query_nodeid
	fwrite(&(proof->query_nodeid), sizeof(ULONG), 1, fp);

	// answer
	fwrite(&(proof->answer), sizeof(UINT), 1, fp);

	// num_proof_nodeid
	fwrite(&(proof->num_proof_nodeid), sizeof(UINT), 1, fp);

	//ULONG *proof_nodeid_list;
	fwrite(proof->proof_nodeid_list, sizeof(ULONG), proof->num_proof_nodeid, fp);

	//char **proof_label_list;
	for(i=0; i<proof->num_proof_nodeid; i++) {
#if 0
		if(i == 7) {
			int k;
			for(k=0; k<LABEL_BUFFER_LEN; k++){
				if(proof->proof_label_list[i][k] != proof->proof_label_list[i+1][k])
					printf("different (%x, %x)\n", proof->proof_label_list[i][k], proof->proof_label_list[i+1][k]);
				else
					printf("%x\n", proof->proof_label_list[i][k]);
			}

			fwrite(proof->proof_label_list[i+1], LABEL_BUFFER_LEN, 1, fp);
		}
		else {
			fwrite(proof->proof_label_list[i], LABEL_BUFFER_LEN, 1, fp);
		}
#else
		fwrite(proof->proof_label_list[i], LABEL_BUFFER_LEN, 1, fp);
#endif

		//printf("*****%d:%llu\n", i, proof->proof_nodeid_list[i]);
		//gsl_vector_fprintf(stdout, decode_vector_buffer(proof->proof_label_list[i], LABEL_BUFFER_LEN), "%f");
	}

	fclose(fp);
}


#if 0
/*****************************************************************************
*
*	range query()
*
*****************************************************************************/
RangeProof *process_range_query(ULONG start_nodeid, ULONG end_nodeid) {
	RangeProof *proof = malloc(sizeof(RangeProof));
	UINT num_answer_node = 0, num_proof_node = 0;

	GList *answer_nodeid_list = NULL;
	GHashTable *nodeid_table  = g_hash_table_new(g_int64_hash, g_int64_equal);


	DEBUG_TRACE(("process_range_query(%llu, %llu) init\n", start_nodeid, end_nodeid));

	/** 1. get nodeids of leaf nodes in the given range and put into hash table **/
	smysql_get_leaf_nodeids_in_range(start_nodeid, end_nodeid, nodeid_table);
	if(g_hash_table_size(nodeid_table) == 0) {
		DEBUG_TRACE(("No nodes in the range\n"));
		free(proof);
		g_hash_table_destroy(nodeid_table);
		return NULL;
	}

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
	proof->answer_nodeid_list = malloc(sizeof(ULONG) * num_answer_node);
	proof->answer_list = malloc(sizeof(UINT) * num_answer_node);

	build_range_answer(proof, answer_nodeid_list);


	// proof
	num_proof_node = g_hash_table_size(nodeid_table);
	proof->num_proof_nodeid = num_proof_node;
	proof->proof_nodeid_list = malloc(sizeof(ULONG) * num_proof_node);
	proof->proof_label_list = malloc(sizeof(char *) * num_proof_node);

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
		//printf("%llu\n", curr_nodeid);
		//printf("%d\n", g_list_length(answer_nodeid_list));

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

			//printf("(%llu, %llu, %llu) %d\n", curr_nodeid ^ (ULONG)1, curr_nodeid, *(ULONG *)(answer_nodeid_list->data), g_hash_table_size(nodeid_table));

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

		//printf("**%u (%llu, %u)\n", node_count, proof->answer_nodeid_list[node_count], proof->answer_list[node_count]);

		node_count++;
		answer_nodeid_list = answer_nodeid_list->next;
	}


}


void build_range_proof(RangeProof *proof, GHashTable *nodeid_table) {
	GList *nodeid_list = g_hash_table_get_keys(nodeid_table);
	UINT node_count = 0;
	ULONG curr_nodeid = 0;


	DEBUG_TRACE(("build_range_proof()\n"));

	while(nodeid_list) {
		char *label_buffer = NULL;

		curr_nodeid = *(ULONG *)(nodeid_list->data);

		//printf("curr_nodeid:%llu\n", curr_nodeid);

		/** nodeid */
		proof->proof_nodeid_list[node_count] = curr_nodeid;

		/** label_buffer */
		proof->proof_label_list[node_count] = malloc(LABEL_BUFFER_LEN);

		label_buffer = smysql_get_node_label(curr_nodeid);

		if(label_buffer) {
			//printf("label exists!!\n");
			memcpy(proof->proof_label_list[node_count], label_buffer, LABEL_BUFFER_LEN);
		}
		else {
			memset(proof->proof_label_list[node_count], 0, LABEL_BUFFER_LEN);
		}
		free(label_buffer);


		node_count++;
		nodeid_list = nodeid_list->next;

	}

}


void write_range_proof(RangeProof *proof, UINT index) {
	char filename[40];
	FILE *fp;
	UINT i = 0;

	DEBUG_TRACE(("write_range_proof(%u, %llu, %llu), num_answer(%u), num_proof(%u)\n", \
			index, proof->start_nodeid, proof->end_nodeid, proof->num_answer_nodeid, proof->num_proof_nodeid));

	sprintf(filename, "./proof/sads_range_proof_%d.dat", index);
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
	for(i=0; i<proof->num_proof_nodeid; i++) {
		fwrite(proof->proof_label_list[i], ELEMENT_LEN, LABEL_LEN, fp);
	}

	fclose(fp);
}
#endif


/********************************************************
*
*	misc.
*
*********************************************************/



/********************************************************
*
*	test functions
*
*********************************************************/
void run_membership_test(char* node_input_filename, int num_query) {
	UINT num_nodes = 0;
	UINT i = 0;
	ULONG *node_list = read_node_input(node_input_filename, &num_nodes);
	MembershipProof *membership_proof;

    struct timeval  tv1, tv2;

	DEBUG_TRACE(("Benchmark Test(Membership Query): num_nodes(%u)\n", num_nodes));

	/** Prover Benchmark **/
	/** update() **/
	printf("update start\n");
    gettimeofday(&tv1, NULL);

	for(i=0; i<num_nodes; i++) {
		update_prover(node_list[i]);

		if( (i+1) % (num_nodes/10) == 0)
			printf("%d/%d done.\n", i+1, num_nodes);
	}

    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));

	/** query() **/
	printf("membership query start\n");
    gettimeofday(&tv1, NULL);

	for(i=0; i<num_query; i++) {
		membership_proof = process_membership_query(node_list[i]);
		write_membership_proof(membership_proof, i);
		if( (i+1) % (num_query/10) == 0)
			printf("%d/%d done.\n", i+1, num_query);

		/** memory free */
	    free_membership_proof(membership_proof);
	}

    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));

}

#if 0
void run_range_test(char* node_input_filename, int num_query) {
	UINT num_nodes = 0;
	UINT i = 0;
	ULONG *node_list = read_node_input(node_input_filename, &num_nodes);
	RangeProof *range_proof;

    struct timeval  tv1, tv2;

	DEBUG_TRACE(("Benchmark Test(Range Query): num_nodes(%u)", num_nodes));

	/** Prover Benchmark **/
	/** update() **/
	printf("update start\n");
    gettimeofday(&tv1, NULL);

	for(i=0; i<num_nodes; i++) {
		update_prover(node_list[i]);
		printf("%d/%d done.\n", i, num_nodes);
	}

    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));


	/** query() **/
	printf("query start\n");
    gettimeofday(&tv1, NULL);

    if(num_nodes > 1) {
    	ULONG start_nodeid, end_nodeid;

    	for(i=0; i<num_query-1; i++) {
    		if(node_list[i] < node_list[i+1]) {
    			start_nodeid = node_list[i];
    			end_nodeid = node_list[i+1];
    		}
    		else {
    			start_nodeid = node_list[i+1];
    			end_nodeid = node_list[i];
    		}
    		range_proof = process_range_query(start_nodeid, end_nodeid);

    		if(range_proof)
    			write_range_proof(range_proof, i);

    		printf("%d/%d done.\n", i, num_nodes);
    	}
	}

    gettimeofday(&tv2, NULL);
    printf ("Total time = %f seconds\n",
             (double) (tv2.tv_usec - tv1.tv_usec)/1000000 +
             (double) (tv2.tv_sec - tv1.tv_sec));

}
#endif

/*****************************************************************************
*
*	main()
**
*****************************************************************************/
int main(int argc, char* argv[]) {

	printf("SADS Prover\n");
	if(argc != 5)
	{
		printf("Usage: ./sads_prover <params_filename> <nodes_input_filename> <test_mode> <num_query>\n");
		exit(-1);
	}
	
	/** initialize() **/
	init_prover(argv[1]);
	//smysql_init();

	/** Benchmark **/
	if(strcmp(argv[3], "membership") == 0) {
		run_membership_test(argv[2], atoi(argv[4]));

		g_hash_table_destroy(label_table);
		g_hash_table_destroy(leaf_value_table);

	}
#if 0
	else if(strcmp(argv[3], "range") == 0) {
		run_range_test(argv[2], atoi(argv[4]));
	}
#endif
	else {
		printf("<test mode>: membership / range\n");
	}

	printf("prover done\n");
	return 0;
}




