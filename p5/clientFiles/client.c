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

//count number of words in buffer
int countWords3 (char* str){
    int OUT = 0;
    int IN = 1;
    int state = OUT;
    int wc = 0;

    while(*str){
        if(*str == ' ' || *str == '\n' || *str == '\t'){
            state = OUT;
        }
        else if (state == OUT){
            state = IN;
            ++wc;
        }
        ++str;
    }
    return wc;
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
void readWriteSocket(int sockfd, char *ipNum){

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

   
    //convert addr to without dots  
    int lenIp = (int)strlen(ipNum);
    for (int i = 0; i < lenIp; i++){
        if (ipNum[i] == '.'){
            ipNum[i] = ',';
        }
    }

    //infinite while loop that exits when user types in quit
    while(1){
        bzero(sendline,MAXLINE); //zeroing out the buffer
        printf("ftp> ");
        fgets(sendline, MAXLINE, stdin); //gets user input

        //get the command
        char sendlineTemp[MAXLINE], command[MAXLINE], extraSend[MAXLINE];
        bzero(sendlineTemp, MAXLINE);
        bzero(command, MAXLINE);
        bzero(extraSend, MAXLINE);
        sprintf(sendlineTemp, "%s", sendline);
        sprintf(extraSend, "%s", sendline);
        char *pch; 
        pch = strtok(sendlineTemp, " ");
        strcpy(command, pch);

        //get the number of words in command
        char sample[MAXLINE];
        bzero(sample, MAXLINE);
        sprintf(sample, "%s", sendline);
        int wordCount = countWords3(sample);

        char dataRecv[MAXLINE]; char dataSend[MAXLINE];
        bzero(dataRecv, MAXLINE);
        bzero(dataSend, MAXLINE);
        bzero(recvline, MAXLINE);
        bzero(sendline, MAXLINE);

        //check each command user has entered and depending on which one,
        //do the appropriate logic 
        if ((strncmp(command, "ls", 2) == 0) && (wordCount == 2)){

            sprintf(sendline, "PORT %s,%d,%d", ipNum, firstNum, secondNum);
            if(send(sockfd, sendline, MAXLINE, 0) < 0){
                printf("send error\n");
                exit(1);
            }
            
            int connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
            if (connfd < 0){
                printf("accept error\n");
                exit(1);
            } 
 
            //recieving 200 OK for POST command 
            bzero(recvline, MAXLINE); 
            if(recv(sockfd, recvline, MAXLINE, 0) < 0){
                printf("send error\n");
                exit(1);
            }
 
            printf("%s\n", recvline);
 
            //send list command with rest of list arguments
            bzero(sendline, MAXLINE);
            sprintf(sendline, "LIST ");
            pch = strtok(NULL, "\n");
            //getting rid of spaces but called filename for simplicity
            char fileName[MAXLINE];
            bzero(fileName, MAXLINE);
            int fileIndex = 0;
            for (int i = 0; i< (int)strlen(pch); i++){
                if (pch[i] != ' '){
                    fileName[fileIndex] = pch[i];
                    fileIndex++;
                }
            }          
            //finish sending
            sprintf(sendline + strlen(sendline), "%s", fileName); 
            send(sockfd, sendline, MAXLINE, 0);
 
        
            //MAYBE USE SELECT => can recieve data on sockfd or connfd
            while(recv(connfd, recvline, MAXLINE, 0) > 0){
                printf("%s", recvline);
                bzero(recvline, MAXLINE);
            }
            bzero(recvline,MAXLINE);
            if (recv(sockfd, recvline, MAXLINE, 0) > 0){
                printf("%s\n", recvline);
            } 

            bzero(sendline, MAXLINE);
            bzero(recvline, MAXLINE); 

        }else if ((strncmp(command, "get", 3) == 0) && (wordCount == 2)){

            sprintf(sendline, "PORT %s,%d,%d", ipNum, firstNum, secondNum);
            if(send(sockfd, sendline, MAXLINE, 0) < 0){
                printf("send error\n");
                exit(1);
            }

            
            int connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
            if (connfd < 0){
                printf("accept error\n");
                exit(1);
            } 
 
            //recieving 200 OK for POST command 
            bzero(recvline, MAXLINE); 
            if(recv(sockfd, recvline, MAXLINE, 0) < 0){
                printf("send error\n");
                exit(1);
            }
 
            printf("%s\n", recvline);
 
            //send list command with rest of list arguments
            bzero(sendline, MAXLINE);
            sprintf(sendline, "RETR ");
            pch = strtok(NULL, "\n");
            //create file name with filename provided by user
            char fileName[MAXLINE];
            bzero(fileName, MAXLINE);
            int fileIndex = 0;
            for (int i = 0; i< (int)strlen(pch); i++){
                if (pch[i] != ' '){
                    fileName[fileIndex] = pch[i];
                    fileIndex++;
                }
            }          
            //finish sending
            sprintf(sendline + strlen(sendline), "%s", fileName); 
            send(sockfd, sendline, MAXLINE, 0);


            //MAYBE USE SELECT => can recieve data on sockfd or connfd
            char comparison[MAXLINE];
            bzero(comparison, MAXLINE);
            sprintf(comparison, "550 File unavailable\n");
            int once = 0;
            FILE *fileptr;
            while(recv(connfd, recvline, MAXLINE, 0) > 0){
                if (strncmp(recvline,comparison,(int)strlen(recvline)) == 0){
                    printf("%s", recvline); 
                    bzero(recvline, MAXLINE); 
                }else if(once == 0){
                    if(access(fileName,F_OK)!=-1){
                        remove(fileName);
                    }

                    fileptr = fopen(fileName,"a");
                    if(fileptr == NULL){
                        printf("file cannot be written to\n");
                        exit(1); //the code should never come here
                    }
                    fputs(recvline, fileptr);
                    fclose(fileptr);
                    once = 1;
                    bzero(recvline, MAXLINE);
                }else{
                    fileptr = fopen(fileName,"a");
                    if(fileptr == NULL){
                        printf("file cannot be written to\n");
                        exit(1); //the code should never come here
                    }
                    fputs(recvline, fileptr);
                    fclose(fileptr); 
                    bzero(recvline, MAXLINE);
                }
            }

            

            bzero(recvline,MAXLINE);
            if (recv(sockfd, recvline, MAXLINE, 0) > 0){
                printf("%s\n", recvline);
            } 

            bzero(sendline, MAXLINE);
            bzero(recvline, MAXLINE); 

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
    readWriteSocket(sockfd, argv[1]);
    return 0;
}
