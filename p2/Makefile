#Makefile extracomment

all: server2.c
	rm -f client server2 *.o
	gcc -Wall server2.c -o server2
	./server2 9999

c: client.c
	gcc -Wall client.c -o client
	./client server-info.txt 2 Makefile

clean: 
	rm -f client server2 *.o

test: test_ex1.py
	python3 test_ex1.py
