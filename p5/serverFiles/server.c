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

#define MAXLINE2 4096 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int     listenfd, connfd,read_size; //the listen and accept file desciptors
struct sockaddr_in servaddr; //the server address
char    sendBuff[MAXLINE2], recvBuff[MAXLINE2]; //the buffer which reads and sends lines
pid_t pid;

/*
 * This method checks the number of arguments when running
 * the server. It makes sure the number of arguments does
 * not exceed 2. An error will be thrown otherwise.
 */
void numArgs(int argc){
    if (argc != 2){
        printf("usage: server <listen-port>\n");
        exit(1);
    }
}

/*
 * This method creates a socket on the server. This
 * is the socket that will be listened on and an error
 * will be thrown if there is a problem creating
 * this socket. If no error, the listenfd is returned.
 */
int createListenSocket(){
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0){
        printf("socket error\n");
        exit(1);
    }
    return listenfd;
}

/*
 * This method initializes the characteristics of the 
 * server and gives it the port number specified by the
 * user.  
 */
void createServer(const char *portnum){
    int port;
    sscanf(portnum,"%d",&port); //converts portnum to int
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port); //sets the port number here
}

/*
 * This method binds the port using the listen file 
 * descriptor. If there is an error with this process
 * an error will be thrown. An error is thrown if that
 * port is being used for other purposes.
 */
void bindServer(int listenfd){
    int bindID = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (bindID < 0){
        printf("bind error\n");
        exit(1);
    } 
}

/*
 * This method listend forever waiting for clients to connect
 * to this server. When a client is connected it is added
 * to the listen queue. An error is thrown otherwise.
 */
void listenServer(int listenfd){
    int listenID = listen(listenfd, LISTENQ); //the listen queue is LISTENQ
    if (listenID < 0){
        printf("listen error\n");
        exit(1);
    } 
}

/*
 * This method uses the accept system call which finds a diff
 * port on the server for the client to communicate with.
 * This is to allow other clients to communicate with the 
 * server's port while the original client can still communicate 
 * with the server. An error is thrown if a problem occurs.
 */
int acceptServer(int listenfd){
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd < 0){
            printf("accept error\n");
            exit(1);
        } 
        return connfd;
}


/*
 * This method is the main meat of this program. It has an
 * infinite while loop, that will continuously read input from
 * the client and execute those commands that were sent to it.
 * It does so by using the popen system call which basically
 * opens a file and inputs the execution into it which later
 * gets sent to a buffer. This buffer is then sent to the client
 * who prints the output onto the screen.
 */
void readWriteServer(int listenfd, const char* portNum){
    while(1){
        
        int connfd = acceptServer(listenfd); 

        pid = fork();
        if (pid == 0){
            while(1){
                printf("in child\n");
                
                int sockfd; 
                int read_size, send_size;
                bzero(recvBuff, MAXLINE2);
                bzero(sendBuff, MAXLINE2);

                read_size = recv(connfd, recvBuff, MAXLINE2, 0);
 
                //get the command
                char recvBuffTemp[MAXLINE2], command[MAXLINE2], rest[MAXLINE2];
                bzero(recvBuffTemp, MAXLINE2);
                bzero(command, MAXLINE2);
                bzero(rest, MAXLINE2);
                sprintf(recvBuffTemp, "%s", recvBuff);
                char *pch; 
                pch = strtok(recvBuffTemp, " ");
                strcpy(command, pch);
                pch = strtok(NULL, "\n");
                strcpy(rest, pch);

                if (strncmp("PORT", command, 4) == 0){ 
                    printf("in port\n");

                    char myIp[MAXLINE2], myPort[MAXLINE2], firstNumAr[MAXLINE2], secondNumAr[MAXLINE2];
                    bzero(myIp, MAXLINE2);
                    bzero(myPort, MAXLINE2);
                    int commaCount = 0;
                    int restlen = (int)strlen(rest);
                    int location = -1;
                    int firstNum, secondNum, realPort;

                    //get ip address in correct format            
                    for(int i = 0; i < restlen; i++){
                        if (commaCount < 4){
                            if(rest[i] == ','){
                                myIp[i] = '.';
                                commaCount++;
                            }else{
                                myIp[i] = rest[i];
                            }
                        }else{
                            location = i;
                            break;
                        }
                    }
                    myIp[(int)strlen(myIp) - 1] = '\0';

                    //get port number in correct format
                    memcpy(myPort, &rest[location], (int)strlen(rest) - 1);
                    char *pch2;
                    pch2 = strtok(myPort, ",");
                    strcpy(firstNumAr,pch2);
                    pch2 = strtok(NULL, "\n");
                    strcpy(secondNumAr, pch2);
                    firstNum = atoi(firstNumAr);
                    secondNum = atoi(secondNumAr);
                    realPort = firstNum * 256 + secondNum;                    

                    //set client socket, its address and connect
                    //set up server data port
                    sockfd = socket(AF_INET, SOCK_STREAM, 0);
                    if (sockfd < 0){
                        printf("socket error\n");
                        exit(1);
                    }
                    struct sockaddr_in servaddr2; //the server address 
                    int port;
                    sscanf(portNum,"%d",&port); //converts portnum to int
                    while(1){
                        port = port - 1; 

                        bzero(&servaddr2, sizeof(servaddr2));
                        servaddr2.sin_family = AF_INET;
                        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
                        servaddr2.sin_port = htons(port); //sets the port number here 

                        if (bind(sockfd, (struct sockaddr *)&servaddr2, sizeof(servaddr2)) < 0) {
                            continue; 
                        }else{
                            break;
                        }
                    }

                    struct sockaddr_in clientaddr; //the server addresss
                    bzero(&clientaddr, sizeof(clientaddr));
                    clientaddr.sin_family = AF_INET;
                    clientaddr.sin_port = htons(realPort); //port number being set
                    //ip address specified below by the user  
                    if (inet_pton(AF_INET, myIp, &clientaddr.sin_addr) <= 0){
                        printf("inet_pton error");
                        exit(1);
                    }
                    if (connect(sockfd, (struct sockaddr *) &clientaddr, sizeof(clientaddr)) < 0){
                        printf("connect error\n");
                        exit(1);
                    } 
                    printf("ipNum:%s myPort:%d\n", myIp, realPort);
                    
                    bzero(sendBuff, MAXLINE2);
                    sprintf(sendBuff, "200 PORT OK");
                    send(connfd, sendBuff, MAXLINE2, 0); 
           

                }else if(strncmp("LIST", command, 4) == 0){
                    printf("in list\n");
                    int errorMsg = 1;
                    int errorMsg2 = 1;
                    FILE *in;
                    extern FILE *popen();
                    char lsCommand[MAXLINE2];
                    bzero(lsCommand, MAXLINE2);
                    sprintf(lsCommand, "ls %s", rest);
                    if (!(in = popen(lsCommand, "r"))) {
                       errorMsg2 = 0; 
                    }else{
                        while((fgets(sendBuff, sizeof(sendBuff), in)) != NULL){
                            errorMsg = 0;
                            send(sockfd, sendBuff, MAXLINE2, 0); 
                            bzero(sendBuff, MAXLINE2);
                        }
                    }
                    if (errorMsg == 1){
                        sprintf(sendBuff, "ls: cannot access '%s': No such file or directory\n", rest);
                        send(sockfd, sendBuff, MAXLINE2, 0);
                    }
                    fclose(in);
                    close(sockfd);
                    bzero(sendBuff, MAXLINE2);
                    if (errorMsg2 == 1){
                        sprintf(sendBuff,"200 LIST OK");
                        send(connfd, sendBuff, MAXLINE2, 0);
                    }else{
                        sprintf(sendBuff,"451 Local Error");
                        send(connfd, sendBuff, MAXLINE2, 0);
                    }

                    bzero(recvBuff, MAXLINE2);
                    bzero(sendBuff, MAXLINE2);
                }else if(strncmp("RETR", command, 4) == 0){
                    printf("in retr\n");
                    int errorMsg = 1;
                    int errorMsg2 = 1;
                    FILE *in;
                    extern FILE *popen();
                    char catCommand[MAXLINE2];
                    bzero(catCommand, MAXLINE2);
                    sprintf(catCommand, "cat %s", rest);
                    if (!(in = popen(catCommand, "r"))) {
                       errorMsg2 = 0; 
                    }else{
                        while((fgets(sendBuff, sizeof(sendBuff), in)) != NULL){
                            errorMsg = 0;
                            send(sockfd, sendBuff, MAXLINE2, 0); 
                            bzero(sendBuff, MAXLINE2);
                        }
                    }
                    if (errorMsg == 1){
                        sprintf(sendBuff, "550 File unavailable\n", rest);
                        send(sockfd, sendBuff, MAXLINE2, 0);
                    }
                    fclose(in);
                    close(sockfd);
                    bzero(sendBuff, MAXLINE2);
                    if (errorMsg2 == 1){
                        sprintf(sendBuff,"200 RETR OK");
                        send(connfd, sendBuff, MAXLINE2, 0);
                    }else{
                        sprintf(sendBuff,"451 Local Error");
                        send(connfd, sendBuff, MAXLINE2, 0);
                    }

                    bzero(recvBuff, MAXLINE2);
                    bzero(sendBuff, MAXLINE2);

            
                }else if(strncmp("STOR", command, 4) == 0){
                    printf("in stor\n");


                }else if(strncmp("QUIT", command, 4) == 0){
                    printf("in quit\n");


                }else if(strncmp("ABOR", command, 4) == 0){
                    printf("in abor\n");


                } 
            }// inf while loop in child
        }else{
            printf("in parent\n");
        }
    }
}

/*
 * This is the class's main which gets executed when this 
 * program is run.
 */
int
main(int argc, char **argv)
{
  numArgs(argc);
  int listenfd = createListenSocket(); //the file desriptor for the socket on server
  createServer(argv[1]);
  bindServer(listenfd);
  listenServer(listenfd);
  readWriteServer(listenfd, argv[1]);
  return 0;
}
