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

/*
 * This method checks to make sure that the number of 
 * arguments is not invalid and above 3. It will
 * throw an error otherwise.
 */
void numArgs(int argc){
    if (argc != 3){
        perror("usage: a.out <IPaddress> <PORTnumber>\n");
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
        perror("socket error\n");
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
    sscanf(portnum,"%d",&port);  //port number specified
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port); //port number being set
    
    //ip address specified below by the user  
    if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0){
        perror("inet_pton error");
        exit(1);
    }
}

/*
 * This method actually tries to connect the client with the server
 * by using the connect system call.
 */ 
void connectSocket(int sockfd){      
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        perror("connect error\n");
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
  int recfd; //the file descriptor for recieving messages
  
  //infinite while loop that exits either during an error
  //or when the client sends an "exit" message
  while(1){
        bzero(sendline,1024); //zeroing out the buffer
        printf("> ");
        fgets(sendline, 1024, stdin); //gets user input
        
        //sends the message to the server 
        if( send(sockfd , sendline , strlen(sendline) , 0) < 0)
        {
            perror("Send failed\n");
            exit(1);
        } 

        //while loop that ends only when the server is done
        //sending its messages to the client            
        while( (recfd = recv(sockfd , recvline , MAXLINE , 0)) > 0)
        {

	    int myLength = strlen(recvline);
	    const char *last_five = &recvline[myLength-5];
	    const char *last_four = &recvline[myLength-4];	    


            //if client enters exit, this allows the
            //client to close its socket and exit
            if (strncmp(last_four,"exit",4) == 0){
              close(sockfd);
              exit(0);
            }


            //this message is for when the server is done
            //sending its messages. It allows the client to 
            //enter its next command.
            if(strncmp(last_five,"empty",5) == 0){
	      int buffLength = strlen(recvline) - 5;
	      printf("%.*s",buffLength,recvline + 0);
              break;
            }


	    fputs(recvline,stdout);
            bzero(recvline,MAXLINE); //zero out buffer


        }
       
        //if there was a recieving error, it will accounted over here 
        if (recfd < 0){
           perror("recv failed on client\n");
           exit(1); 
        }
        if (recfd == 0){
          printf("recfd is equal to zero");
        }

        bzero(recvline,MAXLINE); //the recieving buffer is reset/zeroed
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
