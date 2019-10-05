CC = gcc
CFLAGS = -lm -pthread -D_POSIX_SOURCE -D_GNU_SOURCE

default: clean motdec

all: clean motdec

motdec: motdec.c
	$(CC) motdec.c -o motdec $(CFLAGS)

clean:
	rm -f motdec
