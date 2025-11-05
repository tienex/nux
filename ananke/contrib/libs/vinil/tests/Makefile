# VINIL Test Suite Makefile

CC = cc
CFLAGS = -Wall -Wextra -O2 -I../include -I../../../../include
LDFLAGS = -L../build/lib -lvinil -lm -lpthread

.PHONY: all clean run

all: test_opcodes

test_opcodes: test_opcodes.c
	$(CC) $(CFLAGS) -o test_opcodes test_opcodes.c $(LDFLAGS)

run: test_opcodes
	LD_LIBRARY_PATH=../build/lib ./test_opcodes

clean:
	rm -f test_opcodes
