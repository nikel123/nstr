PROJECT_NAME := libnstr
AUTHOR := Andrej Gelenberg <andrej.gelenberg@udo.edu>
VER_MAJOR := 0
VER_MINOR := 1

all: libnstr.so

libnstr.so: libnstr.o
libnstr.so: LIBS=

tests/test: tests/test.o

CFLAGS += -I. -Werror

CLEAN += tests/test

tests/test bench libnstr.so: Makefile

.PHONY: test do_bench
test: tests/test
	LD_LIBRARY_PATH=. ./tests/test

ntest: tests/ntest
	LD_LIBRARY_PATH=. ./tests/ntest

valgrind: tests/test
	LD_LIBRARY_PATH=. valgrind ./tests/test

gdb: tests/test
	LD_LIBRARY_PATH=. gdb ./tests/test

PERF ?= perf
perf: tests/test
	LD_LIBRARY_PATH=. $(PERF) record ./tests/test

$(eval $(call install_so,libnstr.so))
install: $(DESTDIR)/usr/include/nstr.h
