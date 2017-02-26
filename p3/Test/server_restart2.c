#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "port.h"

#define BUFSIZE 2048
#define MAXLINE2 4096 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int     listenfd, connfd,read_size; //the listen and accept file desciptors
struct sockaddr_in servaddr,remaddr; //the server address
//socklen_t addrlen = sizeof(remaddr); //length of addresses
uint32_t addrlen = sizeof(remaddr); //length of addresses
char    recvBuff[MAXLINE2]; //the buffer which reads lines
char    sendBuff[MAXLINE2]; //buffer which sends lines
time_t ticks; //ticks variable 
int port; //server port number
int recvlen;//# bytes received 
int msgcnt = 0;			/* count # of messages we received */
pid_t pid;
struct timeval tv;

FILE *fileRead; 
FILE *fileRead2; 



int main(){
	printf("1111\n");
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		exit(1);
	}
	printf("2222\n");
	
	    servaddr.sin_family = AF_INET;
	    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    servaddr.sin_port = htons(49032); //sets the port number here

	    if (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind failed");
		exit(1);
	    }

	printf("3333\n");
	  recvlen = recvfrom(fd, recvBuff, MAXLINE2, 0, (struct sockaddr *)&remaddr, &addrlen);
		printf("server not reciving anything??\n");


return 0;
}
