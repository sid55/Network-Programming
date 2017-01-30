#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define MAXLINE 4096

int     sockfd, n;
char    recvline[MAXLINE + 1], sendline[1024];
struct sockaddr_in servaddr;
int port;

void numArgs(int argc){
    if (argc != 3){
        perror("usage: a.out <IPaddress> <PORTnumber>\n");
        exit(1);
    }
}

int createSocket(){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("socket error\n");
        exit(1);
    }
    return sockfd;
}

void createServer(const char *ipaddr, const char *portnum){
    sscanf(portnum,"%d",&port);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);  /* daytime server */
    if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0){
        perror("inet_pton error");
        exit(1);
    }
}

void connectSocket(int sockfd){      
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        perror("connect error\n");
        exit(1);
    }
}

void readWriteSocket(int sockfd){
  //sprintf(sendline, "hello world");
  int recfd;
  while(1){
        bzero(sendline,1024);
        printf("Enter a command > ");
        fgets(sendline, 1024, stdin);
         
        if( send(sockfd , sendline , strlen(sendline) , 0) < 0)
        {
            perror("Send failed\n");
            exit(1);
        } 
            
        if( (recfd = recv(sockfd , recvline , 4097 , 0)) < 0)
        {
            //printf(recvline);
            //printf("\n");
            //printf("Got here");
            perror("recv failed on client\n");
            exit(1); 
        }
        if (strncmp(recvline,"exit",4) == 0){
            close(sockfd);
            exit(0);
        }
        if (recfd == 0){
            printf("success finding empty");
        } 
        printf("Server reply : ");
        printf(recvline);
        printf("\n");
        bzero(recvline,1024);
  }
}


int
main(int argc, char **argv)
{
    numArgs(argc);
    int sockfd = createSocket();
    createServer(argv[1],argv[2]);
    connectSocket(sockfd);
    readWriteSocket(sockfd);
    return 0;
}



