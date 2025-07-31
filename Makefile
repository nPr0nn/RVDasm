
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

rvdasm: rvdasm.c
	$(CC) $(CFLAGS) rvdasm.c -o rvdasm

clean:
	rm -f rvdasm
