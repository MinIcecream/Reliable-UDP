CFLAGS = -Wall -Wextra -std=c11

all: server client

server: server.c packet.c transport.c connection.c
	gcc $(CFLAGS) -o server server.c packet.c transport.c connection.c

client: client.c packet.c transport.c connection.c
	gcc $(CFLAGS) -o client client.c packet.c transport.c connection.c

clean:
	rm -f server client