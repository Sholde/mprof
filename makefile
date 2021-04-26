CC=mpicc
CFLAGS=-Wall -Wextra -shared -fPIC
OFLAGS=-O2 -march=native

EXE=mprof
LIB=libmprof.so

.PHONY: all install man clean

all: $(LIB)

$(LIB): $(EXE).c
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ $^

install: man $(LIB)
	@ if which install > /dev/null ; then      \
	  install $(LIB) /usr/local/lib ;          \
	  install $(EXE) /usr/local/bin ;          \
	  if [ -f $(EXE).1 ] ; then                \
	    mkdir -p /usr/local/man/man1 ;         \
	    install $(EXE).1 /usr/local/man/man1 ; \
	  fi                                       \
	else                                       \
	  echo "Sorry, you don't have install" ;   \
	fi

man:
	@ if which pandoc > /dev/null ; then     \
	  pandoc $(EXE).md -s -t man > $(EXE).1; \
	else                                     \
	  echo "Sorry, you don't have pandoc" ;  \
	fi

clean:
	rm -Rf *~ *.o $(LIB)
