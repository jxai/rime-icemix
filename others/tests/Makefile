CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lrime

_ := $(if $(shell pkg-config --exists rime 2>/dev/null && echo ok),, \
       $(error librime not found. Install librime-dev (Debian/Ubuntu) or librime-devel (Fedora) and retry))

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
