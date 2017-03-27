#Makefile

all: client server

client: client.c
	gcc -Wall client.c -o client
	mv client ../bin

server: server.c
	gcc -Wall server.c -o server
	mv server ../bin

clean: 
	rm -f ../client ../server
