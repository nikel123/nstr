PROJECT_NAME := libnstr
AUTHOR := Andrej Gelenberg <andrej.gelenberg@udo.edu>
VER_MAJOR := 0
VER_MINOR := 1

all: libnstr.so libnparse.so

libnparse.so: libnparse.o
libnparse.so: LIBS=-lnstr

libnstr.so: libnstr.o
libnstr.so: LIBS=

tests/test: libnstr.so.$(VER_MAJOR)
tests/test: tests/test.o
tests/test: LDFLAGS=-L.
tests/test: LIBS=-lnstr -lnparse

CFLAGS += -I. -Werror

CLEAN += tests/test

tests/test bench libnstr.so libnparse.so: Makefile

.PHONY: test do_bench
test: tests/test
	LD_LIBRARY_PATH=. ./tests/test

ntest: tests/ntest
	LD_LIBRARY_PATH=. ./tests/ntest

valgrind: tests/test
	LD_LIBRARY_PATH=. valgrind --leak-check=full ./tests/test

gdb: tests/test
	LD_LIBRARY_PATH=. gdb ./tests/test

PERF ?= perf
perf: tests/test
	LD_LIBRARY_PATH=. $(PERF) record ./tests/test

$(eval $(call install_so,libnstr.so libnparse.so))
install: $(DESTDIR)/usr/include/nstr.h $(DESTDIR)/usr/include/parse.h
