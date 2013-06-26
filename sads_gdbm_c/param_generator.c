#include <gsl/gsl_matrix.h>
#include <gsl/gsl_rng.h>
#include <math.h>
#include <stdio.h>

#include "param_generator.h"

void initialize(UINT kParam, UINT nParam)
{
	k = kParam;
	n = nParam;

	q = n*k*log2(k);
	log_q_ceil = ceil(log2(q));
	
	mu = 2 * k * log_q_ceil;
	beta = n * sqrt(mu);
}

void init_LR()
{
	int i, j;
	gsl_rng *r;

	gsl_rng_env_setup();
	r = gsl_rng_alloc(gsl_rng_default);	

	m = mu/2;
	L = gsl_matrix_alloc(k, m);
	R = gsl_matrix_alloc(k, m);
	
		
	for (i = 0; i < k; i++)
		for (j = 0; j < m; j++)
		{
			gsl_matrix_set(L, i, j, gsl_rng_get(r) % q);
			gsl_matrix_set(R, i, j, gsl_rng_get(r) % q);
		}
}

void output_params()
{	
	FILE *fp;
	char filename[100];

	sprintf(filename, "./param/sads_param_k%d_n%d.dat", k, n);
	fp = fopen(filename, "w");

	
	fprintf(fp, "%d\n", k);
	fprintf(fp,"%d\n", n);
	fprintf(fp,"%d\n", q);
	fprintf(fp,"%d\n", mu);
	fprintf(fp,"%d\n", beta);

	gsl_matrix_fprintf(fp, L, "%f");
	fprintf(fp,"\n");
	
	gsl_matrix_fprintf(fp, R, "%f");
	fprintf(fp,"\n");

	fclose(fp);
}

void free_params()
{
	gsl_matrix_free(L);
	gsl_matrix_free(R);
}
	

int main(int argc, char* argv[])
{
	//printf("SADS Parameters Generator\n");
	if(argc != 3)
	{
		printf("Usage: ./sads_gen_params <k> <n>\n");
		exit(-1);
	}

	/** Parameters */
	initialize(atoi(argv[1]), atoi(argv[2]));	
	init_LR();
	output_params();
	free_params();

	printf("k:%d\n", k);
	printf("n:%d\n", n);
	printf("Matrix dim (%d, %d)\n", k, m);

	return 0;
}
