CC=g++
CPPFLAGS=-Wall -Wextra
LDFLAGS=
LDLIBS=

DEPS=transport.o io.o

all: server client

server: server.o $(DEPS)
	$(CC) server.o $(DEPS) -o server $(LDLIBS)

client: client.o $(DEPS)
	$(CC) client.o $(DEPS) -o client $(LDLIBS)

clean:
	@rm -rf server client *.bin *.o
