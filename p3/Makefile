#Makefile

all: udp-send udp-recv

udp-send: udp-send.c 
	gcc -pthread udp-send.c -o client 

udp-recv: udp-recv.c 
	gcc -pthread udp-recv.c -o server

clean:
	rm -f client server  
