#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <cstdlib>

#include "param_generator.h"

using namespace Eigen;
using namespace std;


void initialize(int securityLevel)
{
	if(securityLevel == 128) {
		k = 23;
		m = 1288;
		q = 72057594037928017;	// 56-bit
		log_q = 56;
	}
	else if(securityLevel == 256) {
		k = 49;
		m = 2597;
		q = 9007199254740997;	// 53-bit
		log_q = 53;
	}
	else {
		printf("wrong parameter: %ud\n", securityLevel);
		exit(-1);
	}

}

void init_LR()
{
	L.resize(k,m);
	R.resize(k,m);

	for (UINT i = 0; i < k; i++)
	{
		for (UINT j = 0; j < m; j++)
		{
			//L(i,j) = ((ULONG)rand()*(ULONG)rand() + (ULONG)rand()) % q;
			//R(i,j) = ((ULONG)rand()*(ULONG)rand() + (ULONG)rand()) % q;

			L(i,j) = (ULONG)rand();
			R(i,j) = (ULONG)rand();
		}
	}
}

void output_params(int securityLevel)
{	
	char filename[100];
	ofstream fp;

	sprintf(filename, "./param/sads_param_%d.dat", securityLevel);
	fp.open(filename);

	fp << k << endl;
	fp << m << endl;
	fp << q << endl;
	fp << log_q << endl;

	for(UINT i=0; i<k; i++)
		for(UINT j=0; j<m; j++)
			fp << L(i, j) << endl;

	for(UINT i=0; i<k; i++)
		for(UINT j=0; j<m; j++)
			fp << R(i, j) << endl;

	fp.close();
}


#if 0
void free_params()
{
	gsl_matrix_free(L);
	gsl_matrix_free(R);
}
#endif

#if 0
int is_Prime(ULONG number)
{
  if ((number & 1) == 0)
  {
    return (number == 2);
  }

  ULONG limit = (ULONG) sqrt(number);
  ULONG i;

  for (i = 3; i <= limit; i += 2)
  {
    if ((number % i) == 0)
    {
      return 0;
    }
  }
  return 1;
}
#endif

int main(int argc, char* argv[])
{
	int securityLevel;

	//printf("SADS Parameters Generator\n");
	if(argc != 2)
	{
		printf("Usage: ./sads_gen_params <128|256>\n");
		exit(-1);
	}

#if 0
	ULONG q_candidate = powl(2, 56);
	ULONG i;
	for(i=q_candidate+1; i<q_candidate*2; i+=2) {
		if(is_Prime(i)) {
			printf("%llu", i);
			return 0;
		}
	}
#endif

	securityLevel = atoi(argv[1]);

	/** Parameters */
	initialize(securityLevel);
	init_LR();
	output_params(securityLevel);
	//free_params();

	printf("k:%d\n", k);
	printf("q:%llu\n", q);
	printf("Matrix dim (%d, %d)\n", k, m);

	return 0;
}
