RIME_SRC  = ../../../librime/src
CC        = gcc
CFLAGS    = -I$(RIME_SRC) -Wall -Wextra -g
# Link against the versioned .so directly (librime-dev not installed)
LIBRIME   ?= /usr/lib/x86_64-linux-gnu/librime.so.1
LDFLAGS   = $(LIBRIME)

.PHONY: all test clean

all: e2e_test probe

e2e_test: e2e_test.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

probe: probe.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: e2e_test
	./e2e_test

# Run only one schema, e.g.:  make test-flypy
test-%: e2e_test
	./e2e_test -s $*

clean:
	rm -f e2e_test probe
