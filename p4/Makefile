#Makefile

all: proxy.c proxy3.c
	gcc proxy.c -o proxyFake
	gcc -pthread proxy3.c -o proxy

clean: 
	rm -f proxy proxyFake
