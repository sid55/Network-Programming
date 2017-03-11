#Makefile

all: proxy 

udp-send: proxy.c 
	gcc -pthread proxy.c -o proxy

clean:
	rm -f proxy 
