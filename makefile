CC=mpicc
CFLAGS=-Wall -Wextra -shared -fPIC
OFLAGS=-O2 -march=native

SRC=src
DOCS=docs

MPROF=mprof
LIB=libmprof.so
MAN=mprof.1

.PHONY: all install man clean

all: $(LIB)

$(LIB): $(SRC)/$(MPROF).c $(SRC)/$(MPROF).h
	$(CC) $(CFLAGS) $(OFLAGS) -o $@ $<

install: $(LIB) man
	@if which install > /dev/null ; then                                   \
		install $(LIB) /usr/local/lib ;                                \
		install $(MPROF).in /usr/local/bin/$(MPROF) ;                  \
		mkdir -p /usr/local/man/man1 ;                                 \
		install $(DOCS)/$(MAN) /usr/local/man/man1 ;                   \
	else                                                                   \
		echo "Sorry, you haven't install command,"                     \
		" you can't install it automatically" ;                        \
	fi

man:
	@if which pandoc > /dev/null ; then                                    \
		pandoc $(DOCS)/$(MPROF).md -s -t man > $(DOCS)/$(MAN);         \
	else                                                                   \
		echo "Sorry, because you haven't pandoc,"                      \
		" you can't generate a new manpage" ;                          \
	fi

clean:
	rm -Rf *~ *.o $(LIB)
