CFLAGS = -Wall -Wextra -std=c11

all: server client

server: server.c
	gcc $(CFLAGS) -o server server.c

client: client.c
	gcc $(CFLAGS) -o client client.c

clean:
	rm -f server client