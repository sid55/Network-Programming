#Makefile

all: udp-send udp-recv

udp-send: udp-send.c 
	gcc -pthread udp-send.c -o client
	mv client ../bin 

udp-recv: udp-recv.c 
	gcc -pthread udp-recv.c -o server
	mv server ../bin

clean:
	rm -f ../bin/client ../bin/server  
