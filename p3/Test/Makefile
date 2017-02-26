#Makefile

all: udp-send udp-recv

udp-send: udp-send.c 
	gcc -pthread udp-send.c -o client 

udp-recv: udp-recv.c 
	gcc -pthread udp-recv.c -o server

restart: clean clientRestart serverRestart serverRestart2

clientRestart: client_restart.c 
	gcc -pthread client_restart.c -o clientRestart

serverRestart: server_restart.c 
	gcc -pthread server_restart.c -o serverRestart

serverRestart2: server_restart2.c 
	gcc -pthread server_restart2.c -o serverRestart2

clean:
	rm -f client server clientRestart serverRestart serverRestart2
