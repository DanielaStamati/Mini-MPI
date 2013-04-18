CC  = gcc
FLG = -g -Wall -o
EXE = mpirun
SRC = mpi*.c

all: clean build

build:
  $(CC) -fPIC -g3 -c $(SRC) -lrt
	$(CC) -shared -g3 *.o -o libsompi.so -lrt
	$(CC) -L. -Wall -g3 -o $(EXE) main.c -lsompi -lrt

clean:
	rm -rf *~ $(EXE) *.o
