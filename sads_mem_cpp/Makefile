CFLAGS=-g `pkg-config --cflags --libs glib-2.0` -O3

INCLUDES=-I/usr/local/include/eigen3 -I/usr/include/glib-2.0

#LIBS=-lgsl -lgslcblas -lm 
#LIBS=-lgsl -lcblas -latlas -lm
#LIBS_MYSQL=-L/usr/lib/x86_64-linux-gnu -lmysqlclient -lpthread -lz -lm -lrt -ldl

#DEBUG=-DSADS_DEBUG -g
WARN=-Wall
#PROFILE=-g -pg

all: param_generator.o prover.o verifier.o

param_generator.o:	param_generator.cpp
	g++ -o param_generator.o param_generator.cpp $(INCLUDES) $(LIBS) $(WARN)

prover.o:	prover.cpp sads_common.cpp 
	g++ -o prover.o prover.cpp sads_common.cpp $(CFLAGS) $(INCLUDES) $(LIBS) $(LIBS_GDBM) $(WARN) $(DEBUG) $(PROFILE)

verifier.o: verifier.cpp sads_common.cpp
	g++ -o verifier.o verifier.cpp sads_common.cpp $(CFLAGS) $(INCLUDES) $(LIBS) $(LIBS_GDBM) $(WARN) $(DEBUG) $(PROFILE)


clean:
	rm -f ./*.o
