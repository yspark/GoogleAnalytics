CFLAGS=-I/usr/include/mysql -I/usr/include/glib-2.0 -DBIG_JOINS=1  -fno-strict-aliasing  -g `pkg-config --cflags --libs glib-2.0` -O3
LIBS=-lgsl -lgslcblas -lm 
#LIBS=-lgsl -lcblas -latlas -lm
#LIBS_MYSQL=-L/usr/lib/x86_64-linux-gnu -lmysqlclient -lpthread -lz -lm -lrt -ldl

#DEBUG=-DSADS_DEBUG -g
#PROFILE=-g -pg
WARN=-Wall

all: param_generator.o prover.o verifier.o

param_generator.o:	param_generator.c
	gcc -o param_generator.o param_generator.c $(LIBS) $(WARN)

prover.o:	prover.c sads_common.c
	gcc -o prover.o prover.c sads_common.c $(CFLAGS) $(LIBS) $(LIBS_MYSQL) $(WARN) $(DEBUG) $(PROFILE)

verifier.o: verifier.c sads_common.c
	gcc -o verifier.o verifier.c sads_common.c $(CFLAGS) $(LIBS) $(LIBS_MYSQL) $(WARN) $(DEBUG) $(PROFILE)

clean:
	rm -f ./*.o
