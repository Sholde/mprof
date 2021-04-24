CC=mpicc
CFLAGS=-Wall -Wextra -shared -fPIC
OFLAGS=-O2 -march=native

.PHONY: all install clean

all: libmprof.so

libmprof.so: mprof.c
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ $^

install: libmprof.so
	install libmprof.so /usr/local/lib
	install mprof /usr/local/bin

clean:
	rm -Rf *~ *.o libmprof.so
