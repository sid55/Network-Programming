#Makefile extracomment

client: client.c
	gcc -Wall client.c -o client

server: server2.c
	gcc -Wall server2.c -o server2

clean: 
	rm -f client server2 *.o

test: test_ex1.py
	python3 test_ex1.py
