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
char    recvBuff[MAXLINE2]; //the buffer which reads and sends lines
char    sendBuff[MAXLINE2];
time_t ticks; //ticks variable 
int port; //server port number

FILE *fileRead; 

/*
 * This method checks the number of arguments when running
 * the server. It makes sure the number of arguments does
 * not exceed 2. An error will be thrown otherwise.
 */
void numArgs(int argc){
    if (argc != 2){
        perror("usage: a.out <PORTnumber>\n");
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
        perror("socket error\n");
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
        perror("bind error\n");
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
        perror("listen error\n");
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
        printf("connfd before is: %d\n",connfd);
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd < 0){
            perror("accept error\n");
            exit(1);
        }
        printf("connfd after is: %d\n",connfd); 
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
void readWriteServer(int connfd, int listenfd){
int bytesToRead;
int position;
int sendfd;
int setForExit = 0;
printf("got before bxeroing\n");
bzero(recvBuff,MAXLINE2);
bzero(sendBuff,MAXLINE2);
printf("got here with no prob\n");
  while( (read_size = recv(connfd , recvBuff , MAXLINE2 , 0)) > 0 )
    {
	//this int value will be used to ensure that
	//that fgets doesnt get called twice when only is
	//supposed to get called once

        //if user enters nothing or for other cases like
        //that, the if else send a message called 
        //"empty" back to the client 
        if(strncmp(recvBuff,"exit",4) == 0){
              printf("Got into exit section\n");
              sprintf(sendBuff,"exit");
              bzero(recvBuff,MAXLINE2);
              write(connfd,sendBuff,strlen(sendBuff));
              bzero(sendBuff,MAXLINE2);
              fclose(fileRead);
        }else if(strncmp(recvBuff,"filename",7) == 0){
              printf("got into filename section: %s\n",recvBuff);
              bzero(sendBuff,MAXLINE2);
              char subBuff[200];
              memcpy(subBuff, &recvBuff[8], strlen(recvBuff));
              fileRead = fopen(subBuff,"r");
              if (fileRead == NULL){
                  perror("The file you are trying to read does not exist\n");
                  sprintf(sendBuff,"errorFile");
                  sendfd = write(connfd, sendBuff, strlen(sendBuff));
                  if(sendfd < 0){
                    perror("send error");
                    exit(1);
                  }
                  setForExit = 1;
                  break; 
              }else{
                  /*
                   * Size of file being read set in global variable
                   */
                  fseek(fileRead,0L,SEEK_END);
                  int fileSize = ftell(fileRead);
                  rewind(fileRead);
            
                  bzero(recvBuff,MAXLINE2);
        
                  sprintf(sendBuff,"sizeFile");
                  sprintf(sendBuff + strlen(sendBuff),"%d",(long)fileSize);
                  printf("the sendBuff is: %s\n",sendBuff); 
                  sendfd = write(connfd, sendBuff, strlen(sendBuff));
                  if(sendfd < 0){
                    perror("send error");
                    exit(1);
                  }
                  printf("the sendid is: %d\n",sendfd);
              }
              fclose(fileRead);
              bzero(sendBuff,MAXLINE2); 
              bzero(recvBuff,MAXLINE2);
        }else if(strncmp(recvBuff,"openfile",7) == 0){
              printf("got into filename section: %s\n",recvBuff);
              bzero(sendBuff,MAXLINE2);
              char subBuff5[4097];
              memcpy(subBuff5, &recvBuff[8], strlen(recvBuff));
              fileRead = fopen(subBuff5,"r");
              if (fileRead == NULL){
                  perror("The file you are trying to read does not exist\n");
                  exit(1);
              }
            
              bzero(recvBuff,MAXLINE2);
              bzero(sendBuff,MAXLINE2);
              sprintf(sendBuff,"openFile2");
              sendfd = write(connfd, sendBuff, strlen(sendBuff));
              if(sendfd < 0){
                perror("send error");
                exit(1);
              }


              bzero(sendBuff,MAXLINE2); 
              bzero(recvBuff,MAXLINE2);
        }else if(strncmp(recvBuff,"size",4) == 0){
              printf("the recvbuff in size is: %s\n",recvBuff);
              bzero(sendBuff,MAXLINE2);
              char subBuff2[4097];
              memcpy(subBuff2, &recvBuff[4], strlen(recvBuff));
              bytesToRead = atoi(subBuff2);

              bzero(recvBuff,MAXLINE2);

              sprintf(sendBuff,"needPos");
              sendfd = write(connfd, sendBuff, strlen(sendBuff));
              if(sendfd < 0){
                perror("send error");
                exit(1);
              }
              bzero(subBuff2,MAXLINE2);
              bzero(recvBuff,MAXLINE2);
        }else if(strncmp(recvBuff,"position",8) == 0){
              bzero(sendBuff,MAXLINE2);
              char subBuff3[4097];
              memcpy(subBuff3, &recvBuff[8], strlen(recvBuff));
              position = atoi(subBuff3);
            printf("the position is: %d\n", position);

            //anothr change?
            if (fileRead == NULL){
                printf("FOUND MY ERROR\n");
                exit(1);
            }
            rewind(fileRead);
            fseek(fileRead,position,SEEK_SET);
            printf("Success in fseek 0th time\n");
                   
            int writefd = 0;
            int newLen = 0;    
            int bytesDivisible = 0;
            int bytesRemainder = 0;    

            //fix issue?
            //bytesToRead--;

            bzero(sendBuff,MAXLINE2);
            bzero(recvBuff,MAXLINE2);
            bytesDivisible = bytesToRead/MAXLINE2;
            bytesRemainder = bytesToRead%MAXLINE2;
            printf("\n\n\n11111111111111111111111111\n\n\n");
            printf("the bytesToRead is: %d\n",bytesToRead);
            printf("the bytesRemainder is: %d\n",bytesRemainder);
            printf("the bytesDivisible is: %d\n",bytesDivisible);
            printf("the position is: %d\n",position);
            int temp = 0;
            while(temp <= bytesDivisible){
                bzero(sendBuff,MAXLINE2);
                if(temp == 0){
                    printf("came into temp==0\n");
                    fseek(fileRead,position,SEEK_SET);
                    newLen = fread(sendBuff,sizeof(char),bytesRemainder,fileRead);
                    fseek(fileRead,position + bytesRemainder,SEEK_SET);
                    printf("the newLen is: %d\n",newLen);
                    printf("able to do fseek first time to: %d\n", position + bytesRemainder);
                    if (temp == bytesDivisible){
                        printf("came into here early!!\n");
                        setForExit = 1;
                    }
                }else{
                    printf("came into temp!=0\n");
                    fseek(fileRead,position + bytesRemainder + ((temp - 1) * MAXLINE2),SEEK_SET);
                    newLen = fread(sendBuff,sizeof(char),MAXLINE2,fileRead);
                    printf("the newlen2 is: %d\n",newLen);
                    printf("able to do fseek second time to: %d\n", position + bytesRemainder + ((temp - 1)*MAXLINE2));
                    if (temp == bytesDivisible){
                        printf("came into here early!!\n");
                        setForExit = 1;
                    }
                }
                if ((writefd = write(connfd, sendBuff, strlen(sendBuff))) > 0) {
                    //sendBuff[strlen(sendBuff)] = '\0';
                    //printf("PRINT: %s\n", sendBuff);
                    printf("\n\n\nstuck here?\n\n\n");
                    bzero(sendBuff,MAXLINE2);
                }

                printf("\n\n\n\n\n\nITERATION\n\n\n\n\n\n");

                if(writefd < 0){
                    perror("was not able to write data correctly\n");
                    exit(1);
                }else if(writefd == 0){
                   bzero(sendBuff,MAXLINE2);
                   sprintf(sendBuff,"exit");
                   writefd = write(connfd,sendBuff,strlen(sendBuff));
                   if (writefd < 0){
                        perror("server not able to write");
                        exit(1);
                   }
                   bzero(sendBuff,MAXLINE2);
                }
     
                bzero(sendBuff,MAXLINE2);
                temp++;
            }
            printf("333333333333333333333333333\n");
            printf("recvBuff size is: %d\n",(int)strlen(recvBuff));
            fclose(fileRead); //fclose here?

              bzero(sendBuff,MAXLINE2);
              bzero(recvBuff,MAXLINE2);
        }
        else{
              printf("Came into section not supposed to yet\n");
        }
        printf("sanity check\n");
        bzero(recvBuff,MAXLINE2); //zeroes/resets the buffer
        if(setForExit == 1){
            printf("gets into break early\n");
            break;
        }
    }
    printf("Left while loop\n");
    //this is for when the client is done sending
    //its messages to the server. The client disconnects.     
    if(read_size == 0 || setForExit == 1)
    {
        printf("the read_size is 0\n");
        printf("before close connfd is: %d\n",connfd);
        close(connfd); //closes the file descriptor returned by accept
        printf("after close connfd is: %d\n",connfd);
    }
    else if(read_size < 0)
    {
        perror("recv failed on server\n");
    }
    printf("ending of readWrite method\n");
}

/*
 * This method allows the connection to the server to happen
 * for the client with the accept system call. It then calls
 * the main meat of this program, readWriteServer which is 
 * a method that is briefly explained above. There is a while
 * loop to ensure that when the client exits, a new connection
 * can be established with the server.
 */
void acceptReadWriteServer(int listenfd){
  while(1){
    printf("first part while loop\n");
    int connfd = acceptServer(listenfd);
    printf("second part while loop\n");
    readWriteServer(connfd, listenfd); //contains code for receiving and sending data
    printf("third part of while loop\n");
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
  acceptReadWriteServer(listenfd);
  exit(1);
  return 0;
}
