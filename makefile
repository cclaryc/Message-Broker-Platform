CC = gcc
CFLAGS = -Wall -g

all: server subscriber

server: server.c helpers.h
	$(CC) $(CFLAGS) -o server server.c -lm

subscriber: subscriber.c helpers.h
	$(CC) $(CFLAGS) -o subscriber subscriber.c -lm

clean:
	rm -f server subscriber *.o
