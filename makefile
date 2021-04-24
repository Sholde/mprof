CC=mpicc
CFLAGS=-Wall -Wextra -shared -fPIC
OFLAGS=-O2 -march=native

.PHONY: all clean

all: libmprof.so

libmprof.so: mprof.c
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ $^

clean:
	rm -Rf *~ *.o libmprof.so
