#Makefile

client: client.c
	gcc -pthread client.c -o client
	mv client ../bin

server: server.c
	gcc -Wall server.c -o server
	mv server ../bin

clean: 
	rm -f ../bin/client ../bin/server ../bin/*.o ../bin/resultingFile

test: test_ex1.py
	python3 test_ex1.py
