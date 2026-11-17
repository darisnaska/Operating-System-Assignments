CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -g -O2
LDLIBS  = -ljson-c -lm

weather: main.o weather.o meteo_fetch.o meteo_decode.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c weather.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f weather *.o

.PHONY: clean
