CFLAGS=-O3 -g `pkg-config --cflags --libs glib-2.0`

INCLUDES=-I/usr/local/include/eigen3 -I/usr/include/glib-2.0

# Basic GSL
#LIBS=-lgsl -lgslcblas -lm 	
# GSL + ATLAS
#LIBS=-lgsl -lcblas -latlas -lm

LIBS_GDBM=-lgdbm

#DEBUG=-DSADS_DEBUG -g
WARN=-Wall


all: param_generator.o prover.o verifier.o

param_generator.o:	param_generator.cpp
	g++ -o param_generator.o param_generator.cpp $(INCLUDES) $(LIBS) $(WARN)

prover.o:	prover.cpp sads_common.cpp sads_gdbm.cpp
	g++ -o prover.o prover.cpp sads_common.cpp sads_gdbm.cpp $(CFLAGS) $(INCLUDES) $(LIBS) $(LIBS_GDBM) $(WARN) $(DEBUG) $(PROFILE)

verifier.o: verifier.cpp sads_common.cpp sads_gdbm.cpp
	g++ -o verifier.o verifier.cpp sads_common.cpp sads_gdbm.cpp $(CFLAGS) $(INCLUDES) $(LIBS) $(LIBS_GDBM) $(WARN) $(DEBUG) $(PROFILE)



#all: param_generator.o prover.o verifier.o

#param_generator.o:	param_generator.c
#	gcc -o param_generator.o param_generator.c $(LIBS) $(WARN)

#prover.o:	prover.c sads_common.c sads_gdbm.c
#	gcc -o prover.o prover.c sads_common.c sads_gdbm.c $(CFLAGS) $(LIBS) $(LIBS_GDBM) $(WARN) $(DEBUG) $(PROFILE)

#verifier.o: verifier.c sads_common.c sads_gdbm.c
#	gcc -o verifier.o verifier.c sads_common.c sads_gdbm.c $(CFLAGS) $(LIBS) $(LIBS_GDBM) $(WARN) $(DEBUG) $(PROFILE)

clean:
	rm -f ./*.o
