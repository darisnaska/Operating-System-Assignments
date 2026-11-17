CC      ?= cc
CFLAGS  += -std=c11 -Wall -Wextra -pedantic -O2 -pthread
LDFLAGS += -pthread

all: gwgd gwg

gwgd: gwgd.o tcp.o player.o chlng.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

gwg: gwg.o player.o chlng.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

gwgd.o: player.h chlng.h tcp.h
tcp.o: tcp.h
player.o: player.h chlng.h
chlng.o: chlng.h
gwg.o: player.h chlng.h

clean:
	rm -f *.o gwgd gwg

.PHONY: all clean

