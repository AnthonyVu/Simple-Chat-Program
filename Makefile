CC = gcc -Wall

all: client server

clean:
	rm -f *.o core.* client server

client: client.o
	$(CC) -pthread -o client client.o
server: server.o uthash.h
	$(CC) -o server server.o

client.o: client.c
	$(CC) -c client.c
server.o: server.c
	$(CC) -c server.c
