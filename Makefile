CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

all: memsim

memsim: memsim.c
	$(CC) $(CFLAGS) -o memsim memsim.c

clean:
	rm -f memsim
