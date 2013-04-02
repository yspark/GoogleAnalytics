CFLAGS=-I/usr/include/mysql -DBIG_JOINS=1  -fno-strict-aliasing  -g
LIBS=-lgsl -lgslcblas -lm 
#LIBS=-lgsl -lblas -latlas -lm
LIBS_MYSQL=-L/usr/lib/x86_64-linux-gnu -lmysqlclient -lpthread -lz -lm -lrt -ldl

DEBUG=-DSADS_DEBUG -g
WARN=-Wall

all: param_generator.o prover.o verifier.o

param_generator.o:	param_generator.c
	gcc -o param_generator.o param_generator.c $(LIBS) $(WARN)

prover.o:	prover.c sads_mysql.c sads_common.c
	gcc -o prover.o prover.c sads_mysql.c sads_common.c $(CFLAGS) $(LIBS) $(LIBS_MYSQL) $(WARN) $(DEBUG)

verifier.o: verifier.c sads_mysql.c sads_common.c
	gcc -o verifier.o verifier.c sads_mysql.c sads_common.c $(CFLAGS) $(LIBS) $(LIBS_MYSQL) $(WARN) $(DEBUG)

clean:
	rm -f ./*.o
