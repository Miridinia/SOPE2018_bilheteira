CC = gcc -Wall -pthread -lrt

default: client start server

client.o: client.c
	$(CC) -c client.c -o client.o
 client: client.o
	$(CC) client.o -o client

server.o: server.c 
	$(CC) -c server.c -o server.o
server: server.o
	$(CC) server.o -o server

start.o: start_v2.c
	$(CC) -c start_v2.c -o start.o
start: start.o 
	$(CC) start.o -o start

clean: 
	-rm -f *.o
	-rm -f client
	-rm -f start
	-rm -f server