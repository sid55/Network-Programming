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

#define MAXLINE 4096 //size of buffer

int     sockfd;//the socket file descriptor
char    recvline[MAXLINE], sendline[MAXLINE]; //the recieve and send buffers
struct sockaddr_in servaddr; //the server address

/*
 * This method checks to make sure that the number of 
 * arguments is not invalid and above 3. It will
 * throw an error otherwise.
 */
void numArgs(int argc){
    if (argc != 3){
        printf("usage: client <server-ip> <server-listen-port>\n");
        exit(1);
    }
}

/*
 * This method creates a socket on the client side so that
 * the client can interact with the server on this socket.
 * If a socket cannot be created, an error will be thrown. 
 * If there is no error, the socket file descriptor 
 * will be returned.
 */
int createSocket(){
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("socket error\n");
        exit(1);
    }
    return sockfd;
}

/*
 * This method gives the server information such as the port
 * number specified by the user and the ip address. It allows
 * the client to know information about the server so that the
 * client can communicate with the server.
 */
void createServer(const char *ipaddr, const char *portnum){
    int port;
    sscanf(portnum,"%d",&port);  //port number specified
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port); //port number being set
    
    //ip address specified below by the user  
    if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0){
        printf("inet_pton error");
        exit(1);
    }
}

/*
 * This method actually tries to connect the client with the server
 * by using the connect system call.
 */ 
void connectSocket(int sockfd){      
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        printf("connect error\n");
        exit(1);
    }
}

/*
 * The main logic of this program is in this method. The client 
 * communicates with the server using the send and recv system 
 * calls. It has two buffers, one for sending messages and one
 * for recieving messages. This entire method is put under a while
 * loop to allow the client to continuously send messages to the
 * server and there is a seperate while loop inside this client
 * for receiving messages in case the message being sent by the server
 * will require multiple packets. If there are any problems using
 * those two system calls, errors are thrown respectively.
 */
void readWriteSocket(int sockfd){
  
  //infinite while loop that exits either during an error
  //or when the client sends an "exit" message
  while(1){
        bzero(sendline,MAXLINE); //zeroing out the buffer
        printf("ftp> ");
        fgets(sendline, MAXLINE, stdin); //gets user input
 
        if (strncmp(sendline, "ls", 2) == 0){
            printf("in ls\n");
            send(sockfd, sendline, MAXLINE, 0);
            recv(sockfd, recvline, MAXLINE, 0); 
            printf("recieved: %s\n", recvline); 
        }else if (strncmp(sendline, "get", 3) == 0){
            printf("in get\n"); 
        }else if (strncmp(sendline, "put", 3) == 0){
            printf("in put\n");
        }else if (strncmp(sendline, "quit", 4) == 0){
            printf("in quit\n");
        }else{
            continue;
        }
  }       
}

/*
 * This is the main for this class which executes the
 * methods of the client
 */
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
