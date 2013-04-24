#include <gsl/gsl_matrix.h>
#include <gsl/gsl_rng.h>
#include <math.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	unsigned long a = 1;
	unsigned long b = 1;

	printf("%d\n", sizeof(unsigned long long));
	printf("%d\n", sizeof(unsigned long));
	printf("%d\n", sizeof(unsigned int));

	a = a << 40;


	printf("%lu\n", a);
	printf("%lu\n", (unsigned long)1 << 40);


	return 0;
}
