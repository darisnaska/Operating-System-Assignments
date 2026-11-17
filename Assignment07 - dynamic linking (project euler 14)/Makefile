SHELL = /bin/bash
CC = cc
CFLAGS = -O2 -Wall -Wextra -std=c99

all: libcollatz.so collatz-c

libcollatz.so: collatz.c collatz.h
	$(CC) $(CFLAGS) -fPIC -shared -o $@ collatz.c

collatz-c: main.c libcollatz.so collatz.h
	$(CC) $(CFLAGS) -o $@ main.c -L. -lcollatz -Wl,-rpath,'$$ORIGIN'

bench: all
	@echo "== pure python =="; time python3 collatz_pure.py
	@echo "== pure C (dynamic link) =="; time ./collatz-c
	@echo "== python + ctypes =="; time python3 collatz.py

clean:
	rm -f libcollatz.so collatz-c

.PHONY: all bench clean
