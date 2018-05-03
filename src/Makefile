CC = gcc

default: client start

client.o: client.c
	$(CC) -c client.c -o client.o
 client: client.o
	$(CC) client.o -o client

start.o: start.c
	$(CC) -c start.c -o start.o
start: start.o 
	$(CC) start.o -o start

clean: 
	-rm -f *.o
	-rm -f client
	-rm -f start