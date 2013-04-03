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
	printf("%d\n", k);
	printf("%d\n", n);
	printf("%d\n", q);
	printf("%d\n", mu);
	printf("%d\n", beta);
	
	gsl_matrix_fprintf(stdout, L, "%f");
	printf("\n");
	
	gsl_matrix_fprintf(stdout, R, "%f");
	printf("\n");	
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

	initialize(atoi(argv[1]), atoi(argv[2]));	
	init_LR();
	
	output_params();
	
	free_params();

	return 0;
}
