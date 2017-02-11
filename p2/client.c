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

char    recvline[MAXLINE + 1], sendline[1024]; //the recieve and send buffers
struct sockaddr_in servaddr; //the server address

FILE *fp; //pointer to server list file
char line [1024]; //assumption line length of 1024 bytes
int fileSize = -1; //size of file that has book
int counter = 0; //set after first connection, used to calc position to set fseek on server
int remainderBytes = 0; //set as leftover bytes after dividing file size by minimum.
                        //value is added to only first connection then set to 0
int avgBytes = 0; //set as average number of bytes to send per connection
int setFile = 0;
/*
 * A struct thread that will be used throughout this program to hold info about a thread
 */
typedef struct{
    int thread_id;
    FILE *reader;
    FILE *fp;
    //char ipAddr[1024];
    //char portNumbr[1024];
    int avgBytes;
    int remainderBytes;
    int counter;
    int minimum;
    int fileSize;
    struct sockaddr_in servaddr; //the server address
    int tempVal;
}Thread;

//Thread pointer created -> pointer to an array of structs
Thread *thread_pnt;

/*
 * This method checks to make sure that the number of 
 * arguments is not invalid and above 3. It will
 * throw an error otherwise.
 */
void numArgs(int argc){
    if (argc != 4){
        perror("usage: a.out <server-info.txt> <num-connections> <filename>\n");
        exit(EXIT_FAILURE);
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
    int sockfdtemp = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfdtemp < 0){
        perror("socket error\n");
        exit(1);
    }
    return sockfdtemp;
}

/*
 * This method gives the server information such as the port
 * number specified by the user and the ip address. It allows
 * the client to know information about the server so that the
 * client can communicate with the server.
 */
void createServer(const char *ipaddr, const char *portnum){
    int port;
    //if(minimum > 0){
        sscanf(portnum,"%d",&port);  //port number specified
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port); //port number being set
    
        //ip address specified below by the user  
        if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0){
            perror("inet_pton error");
            exit(1);
        }
    //    minimum--;
    //}else{
    //    perror("Not enough servers to connect to");
    //    exit(EXIT_FAILURE);
    //}
}

/*
 * Makes sure that the server file list provided exists and 
 * that it can be successfully opened.
 */
void checkServerFile(const char *filename){
    if (strncmp(filename,"server-info.txt",15) != 0){
        perror("The file should be server-info.txt\n");
        exit(EXIT_FAILURE);
    }
    fp = fopen(filename,"r");
    if (fp == NULL){
        perror("The file you are trying to open does not exist\n");
        exit(1);
    }
}

/*
 * Finds the next server in list of servers and sets its
 * variables on the client side
 */
void createServerHelper(int sockfd){
    char ipAddr[1024]; //max length of ipaddress - used when opneing file
    char portNumbr[1024]; //max length of portNumbr - used when opening file
    if(fscanf(fp,"%s %s",ipAddr,portNumbr) > 0){
        printf("%s %s \n",ipAddr,portNumbr);
        if (strncmp(ipAddr,"localhost",9) == 0)
            createServer("127.0.0.1",portNumbr);
        else   
            createServer(ipAddr,portNumbr);
            //createServer("127.0.0.1",portNumbr);
    }
}

/*
 * Gets the minimum between the number of the connections the user enters
 * and the number of servers available for usage
 */
void getMin(const char *numConnections){
    int tempNumConnections = -1;
    int tempNumLines = 0;
    int ch = 0;

    /*
     * Find number of lines and set numConnections to int
     */
    sscanf(numConnections,"%d",&tempNumConnections); //converts connection number to int
    while(!feof(fp)){
        ch = fgetc(fp);
        if (ch == '\n'){
            tempNumLines++;
        }
    }
    rewind(fp);

    int minimumTemp;
    //Set the minimum to one of those values
    if(tempNumLines <= tempNumConnections){
        minimumTemp = tempNumLines;
    }else{
        minimumTemp = tempNumConnections;
    }

    //zero connections = throw error
    if (minimumTemp = 0){
        perror("Not enough servers to connect to\n");
        exit(1);
    }

    //set minimum in thread struct
    printf("GETS THIS FAR\n");
    thread_pnt = (Thread*)malloc(minimumTemp*sizeof(Thread));
    int minTemp = minimumTemp -1;
    while(minTemp > -1){
        thread_pnt[minTemp].minimum = minimumTemp;
        minTemp--;
    }

    //set tempVal used for calculation in thread struct   
    thread_pnt[0].tempVal = 0;
    minTemp = minimumTemp - 1;
    while(minTemp > 0){
        thread_pnt[minTemp].tempVal = 1;
        minTemp--;
    }
 
    //thread_pnt[1].thread_id = 5;

}

void getFileSize(){


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
 *
 * This method will call createServerHelper and connectSocket within
 * a loop as well.
 */
void readWriteSocket(Thread threadInfo, const char* fileName){
    int sendfd;
    int recfd; //the file descriptor for recieving messages
    int setBreak = 0; //variable set/unset when wanting to break while loops
    struct sockaddr_in servaddr; //the server address


    int sockfd = createSocket();

    //Create the server with its info below
    int port;
    char ipAddr[1024]; //max length of ipaddress - used when opneing file
    char portNumbr[1024]; //max length of portNumbr - used when opening file
    if(fscanf(fp,"%s %s",ipAddr,portNumbr) > 0){
        printf("%s %s \n",ipAddr,portNumbr);
        if (strncmp(ipAddr,"localhost",9) == 0){
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0){
                perror("inet_pton error");
                exit(1);
            }
        }else{   
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, ipAddr, &servaddr.sin_addr) <= 0){
                perror("inet_pton error");
                exit(1);
            }
        }        
    }


    //connect the the server's socket here
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        perror("connect error\n");
        exit(1);
    }

 
      //infinite while loop that exits either during an error
      //or when the client sends an "exit" message
      while(1){
            //allows sending a string first to the server asking for
            //the file size of the given file
            bzero(sendline,1024);

            if (setFile == 0){
                printf("got into setfile = 0\n");
                sprintf(sendline,"filename");
                sprintf(sendline + strlen(sendline),fileName);
                setFile = 1;
            }else if (setFile == 1){
                printf("got into setFile = 1\n");
                sprintf(sendline,"size");
                sprintf(sendline + strlen(sendline), "%d", (long) (avgBytes + remainderBytes));
                setFile = 2;
            }else if (setFile == 2){
                printf("got into setFile = 2\n");
                sprintf(sendline,"position");
                sprintf(sendline + strlen(sendline), "%d", (long) (((avgBytes + remainderBytes)*threadInfo.tempVal)  + (avgBytes * counter)));
                remainderBytes = 0;
                threadInfo.tempVal = 1;
                setFile = 3;
            }

            //sends the message to the server 
            if( send(sockfd , sendline , strlen(sendline) , 0) < 0)
            {
                perror("Send failed\n");
                exit(1);     
            }             
           
            printf("the sendline after send is: %s\n",sendline);
     
            //while loop that ends only when the server is done
            //sending its messages to the client            
            while( (recfd = recv(sockfd , recvline , MAXLINE , 0)) > 0)
            {

            int myLength = strlen(recvline);
            const char *gotSizeofFile = &recvline[myLength-5];
            const char *last_four = &recvline[myLength-4];	    
            //const char *last_four = &recvline[myLength-4];	    
            //const char *last_four = &recvline[myLength-4];	    
            //const char *last_four = &recvline[myLength-4];	    

            if(strncmp(recvline,"sizeFile",8) == 0){
                printf("came into sizeFile: %s\n",recvline);
                char subBuff[200];
                memcpy(subBuff, &recvline[8], strlen(recvline));
                fileSize = atoi(subBuff);
                avgBytes = fileSize/threadInfo.minimum;
                remainderBytes = fileSize%threadInfo.minimum;
                bzero(recvline,MAXLINE);
                break; 
            }

            if(strncmp(recvline,"needPos",7) == 0){
                printf("came into needPos: %s\n", recvline);
                break;
            }

                //if client enters exit, this allows the
                //client to close its socket and exit
            if (strncmp(last_four,"exit",4) == 0){
                /*
                  int buffLength = strlen(recvline) - 5;
                  printf("%.*s",buffLength,recvline + 0);
                */
                 printf("got into exit\n");
                 close(sockfd);
                 setBreak = 1;
                 break;
              }

              if(setFile == 3){
                printf("%s",recvline); //prints data received onto screen
                bzero(recvline,MAXLINE); //zero out buffer
              }

            }
        
            //printf("THE rec fd is:%d \n", recfd); 
     
            if (setBreak == 1){
                setBreak = 0;
                break;
            }
    
            //if there was a recieving error, it will accounted over here 
            if (recfd < 0){
               perror("recv failed on client\n");
               exit(1); 
            }
            if (recfd == 0){
               perror("recfd is equal to zero");
               exit(1);
            }

            bzero(recvline,MAXLINE); //the recieving buffer is reset/zeroed
        }//close second while loop
    //minimum--;
    //counter++;
    //sockfd = createSocket();

  fclose(fp); //fclose comes here?
  //maybe close socket here too
  printf("ending of thread method\n");
}//close method

/*
 * This is the main for this class which executes the
 * methods of the client
 */
int
main(int argc, char **argv)
{
    numArgs(argc);
    checkServerFile(argv[1]);
    getMin(argv[2]);
    //printf("end of main, size of file read is: %d \n",fileSize);
    //int sockfd = createSocket();

    //createServerHelper();
    //createServerHelper();
    //createServerHelper();

    //int sockfd = createSocket();
    //createServer(argv[1],argv[2]);
    //connectSocket(sockfd);
    readWriteSocket(thread_pnt[0],argv[3]);
    return 0;
}
