#include <stdio.h>
#include <math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

#include "prover.h"
#include "sads_mysql.h"
#include "sads_common.h"

#include <string.h>


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

void initialize(char* filename) {
	read_params(filename);
	
	root_digest = get_initial_digest(k, false);
}




void update_leaf(UINT ip_addr) {
	gsl_vector *digest = NULL, *label = NULL;
	UINT curr_count = 0;

	DEBUG_TRACE(("update_leaf(%d)", ip_addr));

	if(!smysql_get_node_info(ip_addr, &curr_count, digest, label)) {	
		// TODO: inital_digest, initial_label
		char *digest_buffer = NULL, *label_buffer = NULL;
			
		digest = get_initial_digest(k, true);
		label = get_binary_representation(digest);
		
		digest_buffer = encode_vector(digest);
		label_buffer = encode_vector(label);		
		
		smysql_add_leaf(ip_addr, \
				digest_buffer, \
				get_vector_blob_size(digest_buffer), \
				label_buffer, \
				get_vector_blob_size(label_buffer));
		
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
	else {
		// TODO: get current digest or label, and update the path to the root.
		smysql_update_leaf(ip_addr);
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
	
	DEBUG_TRACE(("get_initial_digest\n"));
	//gsl_vector_fprintf(stdout, digest, "%f");
	
	
	return digest;			
}

gsl_vector *get_binary_representation(gsl_vector *digest) {

	gsl_vector *binary = gsl_vector_alloc((digest->size) * log_q_ceil);	
	int i = 0, j = 0, ele = 0;
	
	DEBUG_TRACE(("%d, %d, %d\n", (UINT)digest->size, (UINT)binary->size, log_q_ceil));
	//printf("%lu, %lu, %lu\n", sizeof(size_t), sizeof(gsl_block), sizeof(int));
	//printf("%f, %f, %f\n", *(digest->data), *(digest->data+32), *(digest->data+16));
		
	for(i = 0; i < digest->size; i++) {		
		ele = (UINT)gsl_vector_get(digest, i);
		
		
		for(j = log_q_ceil -1; j >= 0; j--) {

			gsl_vector_set(binary, (i * log_q_ceil) + j, ele & 1);					
			ele = ele >> 1;
		}
		
	}
	
	DEBUG_TRACE(("get_binary_representation\n"));
	//gsl_vector_fprintf(stdout, binary, "%f");
		
	return binary;

}


char *encode_vector(gsl_vector  *vector) {

	int size = sizeof(size_t)*2 + sizeof(double)*(vector->size) + sizeof(gsl_block) + sizeof(int);
	int len = 0;
	char *buffer = malloc(size);
	
	memcpy(buffer, &(vector->size), sizeof(size_t));
	len += sizeof(size_t);
	
	memcpy(buffer+len, &(vector->stride), sizeof(size_t));
	len += sizeof(size_t);

	memcpy(buffer+len, vector->data, sizeof(double)*(vector->size));
	len += sizeof(double)*(vector->size);
	
	memcpy(buffer+len, vector->block, sizeof(gsl_block));
	len += sizeof(gsl_block);
	
	memcpy(buffer+len, &(vector->owner), sizeof(int));
	len += sizeof(int);	

	printf("%d, %d\n", size, len);
	
	return buffer;
}


gsl_vector *decode_vector_blob(char *buffer) {

	gsl_vector *vector = NULL;
	size_t size = 0;
	int len = 0;


	memcpy(&size, buffer, sizeof(size_t));
	len += sizeof(size_t);

	vector = gsl_vector_alloc(size);
	
	memcpy(&(vector->stride), buffer + len, sizeof(size_t));	
	len += sizeof(size_t);
	
	memcpy(vector->data, buffer+len, sizeof(double) * size);
	len += sizeof(double) * size;
	
	memcpy(vector->block, buffer+len, sizeof(gsl_block));
	len += sizeof(gsl_block);
	
	memcpy(&(vector->owner), buffer + len, sizeof(int));
	len += sizeof(int);
	
	//gsl_vector_fprintf(stdout, vector, "%f");
	printf("%d\n", len);
	return vector;	
}

UINT get_vector_blob_size(char *buffer) {
	size_t vector_size = 0;
	UINT total_size = 0;
	
	memcpy(&vector_size, buffer, sizeof(size_t));
	
	total_size = 2*sizeof(size_t) + (sizeof(double) * vector_size) + sizeof(gsl_block) + sizeof(int);
	
	return total_size;
}


void testMatrix() {
	gsl_matrix *A = gsl_matrix_alloc(k, k);
	
	printf("testMatrix\n");
	printf("%d %d\n", (int)L->size1, (int)L->size2);
	printf("%d %d\n", (int)R->size1, (int)R->size2);
	
	gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, L, R, 0.0, A);
	printf("%d %d\n", (int)A->size1, (int)A->size2);
	
	gsl_matrix_fprintf(stdout, A, "%f");

}


int main(int argc, char* argv[]) {
	printf("SADS Prover\n");
	if(argc != 2)
	{
		printf("Usage: ./sads_prover <params_filename>\n");
		exit(-1);
	}
	
	initialize(argv[1]);
	
	smysql_init();

	update_leaf(123456);
	//testMatrix();
		
	return 0;
}
