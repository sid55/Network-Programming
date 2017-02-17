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
char    recvBuff[MAXLINE2]; //the buffer which reads lines
char    sendBuff[MAXLINE2]; //buffer which sends lines
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
        printf("usage: a.out <PORTnumber>\n");
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
 * the client and send back corresponding messages that were sent to it.
 * The server puts the output into a buffer. This buffer is then 
 * sent to the client who prints the output onto the screen.
 */
void readWriteServer(int connfd, int listenfd){
int bytesToRead;
int position;
int sendfd;
int setForExit = 0;
bzero(recvBuff,MAXLINE2);
bzero(sendBuff,MAXLINE2);
  /*
   * Infinite while loop that continues to read info from the 
   * client until no more info needs to be read
   */
  while( (read_size = recv(connfd , recvBuff , MAXLINE2 , 0)) > 0 )
    {

        //This is a series of if else statements expecting 
	//certain inputs from the client. Never goes into
 	//into exit statement 
        if(strncmp(recvBuff,"exit",4) == 0){
              sprintf(sendBuff,"exit");
              bzero(recvBuff,MAXLINE2);
              write(connfd,sendBuff,strlen(sendBuff));
              bzero(sendBuff,MAXLINE2);
              fclose(fileRead);
        }else if(strncmp(recvBuff,"filename",7) == 0){
              bzero(sendBuff,MAXLINE2);
              char subBuff[200];
              memcpy(subBuff, &recvBuff[8], strlen(recvBuff));
              fileRead = fopen(subBuff,"r");
              if (fileRead == NULL){
                  printf("The file you are trying to read does not exist\n");
                  sprintf(sendBuff,"errorFile");
                  sendfd = write(connfd, sendBuff, strlen(sendBuff));
                  if(sendfd < 0){
                    printf("send error\n");
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
                  sprintf(sendBuff + strlen(sendBuff),"%d",fileSize);
                  sendfd = write(connfd, sendBuff, strlen(sendBuff));
                  if(sendfd < 0){
                    printf("send error\n");
                    exit(1);
                  }
              }
              fclose(fileRead);
              bzero(sendBuff,MAXLINE2); 
              bzero(recvBuff,MAXLINE2);
        }else if(strncmp(recvBuff,"openfile",7) == 0){
              bzero(sendBuff,MAXLINE2);
              char subBuff5[4097];
              memcpy(subBuff5, &recvBuff[8], strlen(recvBuff));
              fileRead = fopen(subBuff5,"r");
              if (fileRead == NULL){
                  printf("The file you are trying to read does not exist\n");
                  exit(1);
              }
            
              bzero(recvBuff,MAXLINE2);
              bzero(sendBuff,MAXLINE2);
              sprintf(sendBuff,"openFile2");
              sendfd = write(connfd, sendBuff, strlen(sendBuff));
              if(sendfd < 0){
                printf("send error\n");
                exit(1);
              }


              bzero(sendBuff,MAXLINE2); 
              bzero(recvBuff,MAXLINE2);
        }else if(strncmp(recvBuff,"size",4) == 0){
              bzero(sendBuff,MAXLINE2);
              char subBuff2[4097];
              memcpy(subBuff2, &recvBuff[4], strlen(recvBuff));
              bytesToRead = atoi(subBuff2);

              bzero(recvBuff,MAXLINE2);

              sprintf(sendBuff,"needPos");
              sendfd = write(connfd, sendBuff, strlen(sendBuff));
              if(sendfd < 0){
                printf("send error\n");
                exit(1);
              }
              bzero(subBuff2,MAXLINE2);
              bzero(recvBuff,MAXLINE2);
        }else if(strncmp(recvBuff,"position",8) == 0){
              bzero(sendBuff,MAXLINE2);
              char subBuff3[4097];
              memcpy(subBuff3, &recvBuff[8], strlen(recvBuff));
              position = atoi(subBuff3);

            //if fileRead is null there is a prob
            if (fileRead == NULL){
                printf("file cannot be opened on server\n");
                exit(1);
            }
            rewind(fileRead);
            fseek(fileRead,position,SEEK_SET);
                   
            int writefd = 0;
            int bytesDivisible = 0;
            int bytesRemainder = 0;    

            bzero(sendBuff,MAXLINE2);
            bzero(recvBuff,MAXLINE2);
            bytesDivisible = bytesToRead/MAXLINE2;
            bytesRemainder = bytesToRead%MAXLINE2;

	    //Important debugging info -> print if wanted to
	    /* 
            printf("\n\n\n");
            printf("the bytesToRead is: %d\n",bytesToRead);
            printf("the bytesRemainder is: %d\n",bytesRemainder);
            printf("the bytesDivisible is: %d\n",bytesDivisible);
            printf("the position is: %d\n",position);
	    */

            int temp = 0;
	    

	    /*
    	     * This while loop keeps on going until the temp is
	     * equal to the bytesDivisible
	     */
            while(temp <= bytesDivisible){
                bzero(sendBuff,MAXLINE2);
                if(temp == 0){
                    fseek(fileRead,position,SEEK_SET);
                    fread(sendBuff,sizeof(char),bytesRemainder,fileRead);
                    fseek(fileRead,position + bytesRemainder,SEEK_SET);
		    printf("%s",sendBuff);
                    if (temp == bytesDivisible){
                        setForExit = 1;
                    }
                }else{
                    fseek(fileRead,position + bytesRemainder + ((temp - 1) * MAXLINE2),SEEK_SET);
                    fread(sendBuff,sizeof(char),MAXLINE2,fileRead);
		    printf("%s",sendBuff);
                    if (temp == bytesDivisible){
                        setForExit = 1;
                    }
                }
                if ((writefd = write(connfd, sendBuff, strlen(sendBuff))) > 0) {
                    bzero(sendBuff,MAXLINE2);
                }
	
		//If write was not able to do correctly or if the 
		//the server is done writing certain cases are shown below
                if(writefd < 0){
                    printf("was not able to write data correctly\n");
                    exit(1);
                }else if(writefd == 0){
                   bzero(sendBuff,MAXLINE2);
                   sprintf(sendBuff,"exit");
                   writefd = write(connfd,sendBuff,strlen(sendBuff));
                   if (writefd < 0){
                        printf("server not able to write\n");
                        exit(1);
                   }
                   bzero(sendBuff,MAXLINE2);
                }
     
                bzero(sendBuff,MAXLINE2);
                temp++;
            }
            fclose(fileRead); //fclose here

              bzero(sendBuff,MAXLINE2);
              bzero(recvBuff,MAXLINE2);
        }
        else{
              printf("Not supposed to come here\n");
        }
        bzero(recvBuff,MAXLINE2); //zeroes/resets the buffer
        if(setForExit == 1){
            break;
        }
    }

    //this is for when the client is done sending
    //its messages to the server. The client disconnects.     
    if(read_size == 0 || setForExit == 1)
    {
        close(connfd); //closes the file descriptor returned by accept
    }
    else if(read_size < 0)
    {
        printf("recv failed on server\n");
    }
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
    int connfd = acceptServer(listenfd);
    readWriteServer(connfd, listenfd); //contains code for receiving and sending data
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
  return 0;
}
