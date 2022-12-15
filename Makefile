CFLAGS=-Wall -Wextra -Og -g

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

http-server: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) http-server
