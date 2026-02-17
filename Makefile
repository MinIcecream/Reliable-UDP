CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SOURCES_COMMON = src/packet.c src/transport.c src/connection.c

all: server client

server: src/server.c $(SOURCES_COMMON)
	gcc $(CFLAGS) -o server src/server.c $(SOURCES_COMMON)

client: src/client.c $(SOURCES_COMMON)
	gcc $(CFLAGS) -o client src/client.c $(SOURCES_COMMON)

clean:
	rm -f server client