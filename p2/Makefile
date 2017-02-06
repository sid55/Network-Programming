#Makefile

client: client.c
	gcc -Wall client.c -o client

server: server.c
	gcc -Wall server.c -o server

clean: 
	rm -f client server *.o

test: test_ex1.py
	python3 test_ex1.py
