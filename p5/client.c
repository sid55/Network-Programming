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
#include <ctype.h>
#include <time.h>

#define MAXLINE 4096 //size of buffer
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define LISTENQ 1024

int     sockfd;//the socket file descriptor
char    recvline[MAXLINE], sendline[MAXLINE]; //the recieve and send buffers
struct sockaddr_in servaddr; //the server address
struct sockaddr_in clientDataAddr; 

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

//Count number of words in a buffer
int countWords (const char* str)
{
  int count = 0;

  while (*str != '\0')
  {
    while (*str != '\0' && isblank(*str)) // remove all spaces between words
    {
      str++;
    }
    if(*str != '\0')
    {
      count++;
    }

    while (*str != '\0' && !isblank(*str)) // loop past the found word
    {
      str++;
    }
  }

  return count;
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
void readWriteSocket(int sockfd, const char *ipNum, const char *portNum){

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0){
        printf("socket error\n");
        exit(1);
    }


    int randPort;
    while(1){
        time_t x;
        srand((unsigned) time(&x));
        //srand(getpid())
        randPort = rand() % (65535 + 1 - 40000) + 40000; 

        bzero(&clientDataAddr, sizeof(clientDataAddr));
        clientDataAddr.sin_family = AF_INET;
        clientDataAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        clientDataAddr.sin_port = htons(randPort); //sets the port number here 

        if (bind(listenfd, (struct sockaddr *)&clientDataAddr, sizeof(clientDataAddr)) < 0) {
            continue; 
        }else{
            break;
        }
    }

    int listenID = listen(listenfd, LISTENQ); //the listen queue is LISTENQ
    if (listenID < 0){
        printf("listen error\n");
        exit(1);
    } 
 
    //convert char array to int and get to sep
    //port numbers. Take random prt generated above
    int firstNum;
    int secondNum;
    int tempNum;
    for (int i = 0; i < 8; i++){
        secondNum += CHECK_BIT(randPort,i);
    }
    tempNum = randPort;
    for (int i = 0; i < 8; i++){
        tempNum = tempNum >> 1;
    }
    for (int i = 0; i < 8; i++){
        firstNum += CHECK_BIT(tempNum,i);
    } 

    printf("firstNum:%d secondNum:%d portServOnCli:%d \n", firstNum, secondNum, randPort);

    //infinite while loop that exits when user types in quit
    while(1){
        bzero(sendline,MAXLINE); //zeroing out the buffer
        printf("ftp> ");
        fgets(sendline, MAXLINE, stdin); //gets user input

        char sendlineTemp[MAXLINE], command[MAXLINE];
        bzero(sendlineTemp, MAXLINE);
        bzero(command, MAXLINE);
        sprintf(sendlineTemp, "%s", sendline);

        //get the command
        char *pch; 
        pch = strtok(sendlineTemp, " ");
        strcpy(command, pch);

        //get the number of words in command
        int wordCount = countWords(sendline);

        
        //check each command user has entered and depending on which one,
        //do the appropriate logic 
        if ((strncmp(command, "ls", 2) == 0) && (wordCount == 2)){
            printf("in ls\n");
            send(sockfd, sendline, MAXLINE, 0);
            recv(sockfd, recvline, MAXLINE, 0); 
            printf("recieved: %s\n", recvline); 
        }else if ((strncmp(command, "get", 3) == 0) && (wordCount == 2)){
            printf("in get\n");
            send(sockfd, sendline, MAXLINE, 0);
            recv(sockfd, recvline, MAXLINE, 0); 
            printf("recieved: %s\n", recvline);  
        }else if ((strncmp(command, "put", 3) == 0) && (wordCount == 2)){
            printf("in put\n");
            send(sockfd, sendline, MAXLINE, 0);
            recv(sockfd, recvline, MAXLINE, 0); 
            printf("recieved: %s\n", recvline); 
        }else if ((strncmp(command, "quit", 4) == 0) && (wordCount == 1)){
            printf("in quit\n");
            send(sockfd, sendline, MAXLINE, 0);
            recv(sockfd, recvline, MAXLINE, 0); 
            printf("recieved: %s\n", recvline); 
        }else{
            if ((strncmp(command, "quit", 4) != 0) && (strncmp(command, "put", 3) != 0) && (strncmp(command, "get", 3) != 0) && (strncmp(command, "ls", 2) != 0)){  
                printf("500 Syntax Error\n"); 
            }else{
                printf("501 Syntax Error\n");
            }
            continue;
        }
    }//close inifite loop       
}//close method

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
    readWriteSocket(sockfd, argv[1], argv[2]);
    return 0;
}
