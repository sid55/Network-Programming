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
#include <pthread.h>

#define MAXLINE 4096 //size of buffer

/*
 * NOTE: All these global variables are not being used in the method
 *       called by pthread_create
 */
//CHANGED MAXLINE + 1 to MAXLINE
char    recvline[MAXLINE + 1], sendline[1024]; //the recieve and send buffers
struct sockaddr_in servaddr2; //the server address

int minimum; //leave as global->not used with threads
FILE *fp; //pointer to server list file, leave as global->not used with threads

int fileSize = -1; //size of file that has book
int remainderBytes = 0; //set as leftover bytes after dividing file size by minimum.
                        //value is added to only first connection then set to 0
int avgBytes = 0; //set as average number of bytes to send per connection
/*
 * A struct thread that will be used throughout this program to hold info about a thread
 */
typedef struct{
    char recvline[MAXLINE + 1]; //finished setting
    char sendline[1024]; //finished setting
    int thread_id; //finished setting
    FILE *reader; //used to create sep file to put recvbuff into
    FILE *fp; //finished setting
    //char ipAddr[1024]; //unneeded?
    //char portNumbr[1024]; //uneeded?
    int avgBytes; //finished setting
    int remainderBytes; //finished setting
    int minimum; //finished setting
    int fileSize; //finished setting
    //struct sockaddr_in servaddr; //the server address ---> may not need
    int tempVal; //finished setting
    int setFile; //finished setting
    int counter; //finished setting
    char myFileName[1024];
    char fileOnServer[200];
    char currentLoc;
}Thread;

//Thread pointer created -> pointer to an array of structs
Thread *thread_pnt;
pthread_t *thread_pnt2;
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
        bzero(&servaddr2, sizeof(servaddr2));
        servaddr2.sin_family = AF_INET;
        servaddr2.sin_port = htons(port); //port number being set
    
        //ip address specified below by the user  
        if (inet_pton(AF_INET, ipaddr, &servaddr2.sin_addr) <= 0){
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

//Passing in the filename and the thread object index
void checkServerFile2(const char *filename, Thread threadInfo){
    if (strncmp(filename,"server-info.txt",15) != 0){
        perror("The file should be server-info.txt\n");
        exit(EXIT_FAILURE);
    }
    threadInfo.fp = fopen(filename,"r");
    if (threadInfo.fp == NULL){
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
void getMinAndSetStruct(const char *numConnections, const char *filename, const char* fileOnServer){
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

    printf("tempNumconnections is: %d tempNumlines is: %d",tempNumConnections,tempNumLines);

    int minimumTemp;
    //Set the minimum to one of those values
    if(tempNumLines <= tempNumConnections){
        minimumTemp = tempNumLines;
        minimum = tempNumLines;
    }else{
        minimumTemp = tempNumConnections;
        minimum = tempNumConnections;
    }

    //zero connections = throw error
    if (minimumTemp == 0){
        perror("Not enough servers to connect to\n");
        exit(1);
    }

    //created thread_pnt2 struct and creates instances of pthread_t
    thread_pnt2 = (pthread_t*)malloc(minimumTemp*sizeof(pthread_t));

    //set minimum in thread struct
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

    //set thread_id in this method
    int count = 0;
    minTemp = minimumTemp - 1;
    while(count <= minTemp){
        thread_pnt[count].thread_id = count;
        count++;
    }

    //set setFile in this method
    minTemp = minimumTemp - 1;
    while(minTemp > -1){
        thread_pnt[minTemp].setFile = 0;
        minTemp--;
    }

    //set fp in this piece of code
    count = 0;
    minTemp = minimumTemp - 1;
    while(count <= minTemp){
        if (strncmp(filename,"server-info.txt",15) != 0){
            perror("The file should be server-info.txt\n");
            exit(EXIT_FAILURE);
        }
        thread_pnt[count].fp = fopen(filename,"r");
        if (thread_pnt[count].fp == NULL){
            perror("The file you are trying to open does not exist\n");
            exit(1);
        }
        count++;
    }

    //set reader file pointer in this set of code
    count = 0;
    minTemp = minimumTemp - 1;
    char fileName2[200];
    while (count <= minTemp){
        sprintf(fileName2,"myReadingFilez");
        sprintf(fileName2 + strlen(fileName2), "%d", (long) (count));
        sprintf(thread_pnt[count].fileOnServer,fileName2); 
        thread_pnt[count].reader = fopen(fileName2,"a");
        bzero(fileName2,200);
        count++;
    }

    //set counter in this set of code
    count = 0;
    minTemp = minimumTemp - 1;
    while(count <= minTemp){
        thread_pnt[count].counter = count;
        count++;
    }

    count = 0;
    minTemp = minimumTemp -1;
    while(count <= minTemp){
        sprintf(thread_pnt[count].myFileName,fileOnServer);
        count++;
    }

    count = 0;
    minTemp = minimumTemp -1;
    while(count <= minTemp){
        thread_pnt[count].currentLoc = 0;
        count++;
    }
    //thread_pnt[1].thread_id = 5;

}

void getFileSize(const char* fileName){

    int sockfd = createSocket();
    int recfd;

    //Create the server with its info below
    int port;
    char ipAddr[1024]; //max length of ipaddress - used when opneing file
    char portNumbr[1024]; //max length of portNumbr - used when opening file
    if(fscanf(fp,"%s %s",ipAddr,portNumbr) > 0){
        printf("%s %s \n",ipAddr,portNumbr);
        if (strncmp(ipAddr,"localhost",9) == 0){
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr2, sizeof(servaddr2));
            servaddr2.sin_family = AF_INET;
            servaddr2.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, "127.0.0.1", &servaddr2.sin_addr) <= 0){
                perror("inet_pton error");
                exit(1);
            }
        }else{   
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr2, sizeof(servaddr2));
            servaddr2.sin_family = AF_INET;
            servaddr2.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, ipAddr, &servaddr2.sin_addr) <= 0){
                perror("inet_pton error");
                exit(1);
            }
        }        
    }


    //connect the the server's socket here
    if (connect(sockfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0){
        perror("connect error\n");
        exit(1);
    }

    sprintf(sendline,"filename");
    sprintf(sendline + strlen(sendline),fileName);

    if( send(sockfd , sendline , strlen(sendline) , 0) < 0)
    {
        perror("Send failed\n");
        exit(1);     
    } 


    while( (recfd = recv(sockfd , recvline , MAXLINE , 0)) > 0)
    {

        if(strncmp(recvline,"sizeFile",8) == 0){
            char subBuff[200];
            memcpy(subBuff, &recvline[8], strlen(recvline));
            fileSize = atoi(subBuff);
            avgBytes = fileSize/minimum;
            remainderBytes = fileSize%minimum;
            bzero(recvline,MAXLINE);
            break; 
        }
    }

    close(sockfd);
    rewind(fp);
}

/*
 * This method actually tries to connect the client with the server
 * by using the connect system call.
 */ 
void connectSocket(int sockfd){      
    if (connect(sockfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0){
        perror("connect error\n");
        exit(1);
    }
}

/*
 * Sets avg, total, and remainder in thread_ptr
 */
void setAvgEtc(){
    int count = 1;
    int minTemp = minimum - 1;
    //can check avgbytes etc info here
    //printf("avgBytes is: %d",avgBytes);
    //printf("fileSize is: %d",fileSize);

    thread_pnt[0].fileSize = fileSize;
    thread_pnt[0].remainderBytes = remainderBytes;
    thread_pnt[0].avgBytes = avgBytes;
 
    while(count <= minTemp){
        thread_pnt[count].fileSize = fileSize;
        thread_pnt[count].remainderBytes = 0;
        thread_pnt[count].avgBytes = avgBytes;
        count++;
    }
    printf("THE AVG BYTES 111 IS: %d\n",thread_pnt[0].avgBytes);
    printf("THE AVG BYTES 222 IS: %d\n",thread_pnt[1].avgBytes);
    printf("THE AVG BYTES 333 IS: %d\n",thread_pnt[2].avgBytes);
    printf("THE MINIMUM IS: %d\n",minimum);
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
void *readWriteSocket(void *threadInfoTemp){
    Thread *threadInfo = (Thread *)threadInfoTemp;
    int sendfd;
    int recfd; //the file descriptor for recieving messages
    int setBreak = 0; //variable set/unset when wanting to break while loops
    struct sockaddr_in servaddr; //the server address


    int sockfd = createSocket();

    //Create the server with its info below
    int port;
    char ipAddr[1024]; //max length of ipaddress - used when opneing file
    char portNumbr[1024]; //max length of portNumbr - used when opening file

    int countdown = threadInfo->thread_id;
    while (countdown > 0){
        fscanf(threadInfo->fp,"%s %s",ipAddr,portNumbr);
        countdown--;
    }

    if(fscanf(threadInfo->fp,"%s %s",ipAddr,portNumbr) > 0){
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
            bzero(threadInfo->sendline,1024);

            if (threadInfo->setFile == 0){
                printf("got into setFile = 0\n");
                sprintf(threadInfo->sendline,"openfile");
                sprintf(threadInfo->sendline + strlen(threadInfo->sendline),threadInfo->myFileName);
                threadInfo->setFile = 1; 
            }else if (threadInfo->setFile == 1){
                printf("got into setFile = 1\n");
                bzero(threadInfo->sendline,1024);
                sprintf(threadInfo->sendline,"size");
                sprintf(threadInfo->sendline + strlen(threadInfo->sendline), "%d", (threadInfo->avgBytes + threadInfo->remainderBytes));
                printf("the size of the byts is: %s\n",threadInfo->sendline);
                threadInfo->setFile = 2;
            }else if (threadInfo->setFile == 2){
                printf("got into setFile = 2\n");
                printf("the sendline is: %s\n",threadInfo->sendline);
                printf("the avgBytes is: %d\n",threadInfo->avgBytes);
                printf("the tempVal is: %d\n",threadInfo->tempVal);
                printf("the remainderBytes is: %d\n",threadInfo->remainderBytes);
                printf("the counter is: %d\n",threadInfo->counter);
                printf("\n\n");
                sprintf(threadInfo->sendline,"position");
                //sprintf(threadInfo.sendline + strlen(threadInfo.sendline), "%d", (long) (((threadInfo.avgBytes + threadInfo.remainderBytes)*threadInfo.tempVal)  + (threadInfo.avgBytes * threadInfo.counter)));
                sprintf(threadInfo->sendline + strlen(threadInfo->sendline), "%d", (long) ((threadInfo->avgBytes*threadInfo->counter) + (threadInfo->remainderBytes*threadInfo->tempVal)));
                threadInfo->setFile = 3;
            }

            //sends the message to the server 
            if( send(sockfd , threadInfo->sendline , strlen(threadInfo->sendline) , 0) < 0)
            {
                perror("Send failed\n");
                exit(1);     
            }             
           
            printf("the sendline after send is: %s\n",threadInfo->sendline);
     
            //while loop that ends only when the server is done
            //sending its messages to the client            
            while( (recfd = recv(sockfd , threadInfo->recvline , MAXLINE , 0)) > 0)
            {

            int myLength = strlen(threadInfo->recvline);
            const char *gotSizeofFile = &threadInfo->recvline[myLength-5];
            const char *last_four = &threadInfo->recvline[myLength-4];	    

            if(strncmp(threadInfo->recvline,"needPos",7) == 0){
                printf("came into needPos exit: %s\n", threadInfo->recvline);
                break;
            }

            if(strncmp(threadInfo->recvline,"openFile2",8) == 0){
                printf("came into openFile2 exit: %s\n", threadInfo->recvline);
                break;
            }


                //if client enters exit, this allows the
                //client to close its socket and exit
            if (strncmp(last_four,"exit",4) == 0){
                /*
                  int buffLength = strlen(recvline) - 5;
                  printf("%.*s",buffLength,recvline + 0);
                */
                 printf("\ngot into exit\n");
                 setBreak = 1;
                 break;
              }

              if(threadInfo->setFile == 3){
                printf("\n\n\n\nSEPERATE lenth: %d\n\n\n\n",strlen(threadInfo->recvline));
                printf("----------------------------received buffer is: %s\n",threadInfo->recvline);
                if(threadInfo->recvline[strlen(threadInfo->recvline) - 1690] == '\0'){
                    printf("last character is null\n");
                }else{
                    if(threadInfo->recvline[strlen(threadInfo->recvline) - 1690] == ' '){
                        printf("character is white space\n");
                        printf("num is: %d\n", (int) threadInfo->recvline[strlen(threadInfo->recvline) - 1690]); 
                    }else{
                        printf("the char val for apos space is: %d\n",(int) ' ');
                        printf("character next to last: %d\n", (int) threadInfo->recvline[strlen(threadInfo->recvline)-1690]);
                    }
                }  
                threadInfo->currentLoc = threadInfo->currentLoc + strlen(threadInfo->recvline);
                fseek(threadInfo->reader, threadInfo->currentLoc,SEEK_SET); 
                int val = fwrite(threadInfo->recvline,1,strlen(threadInfo->recvline),threadInfo->reader); 
                //printf("%s",threadInfo.recvline); //prints data received onto screen
                bzero(threadInfo->recvline,MAXLINE); //zero out buffer
                printf("\n\n\n\nEND written: %d\n\n\n\n",val);
              }

            }
        
            //printf("THE rec fd is:%d \n", recfd); 
     
            if (setBreak == 1){
                printf("gets into setBreak\n");
                setBreak = 0;
                break;
            }
    
            //if there was a recieving error, it will accounted over here 
            if (recfd < 0){
               perror("recv failed on client\n");
               exit(1); 
            }
            if (recfd == 0){
               //perror("recfd is equal to zero");
               //exit(1);
               break;
            }

            bzero(threadInfo->recvline,MAXLINE); //the recieving buffer is reset/zeroed
        }//close second while loop
    //minimum--;
    //sockfd = createSocket();

  fclose(threadInfo->fp); //fclose comes here?
  fclose(threadInfo->reader);
  close(sockfd); //look at above when exit is called. it exits automatically?
  printf("ending of thread method\n");
  return NULL; 
}//close method

void combineFiles(){
    int minTemp = minimum - 1;
    int count = 0;
    FILE *myFile;
    char ch;
    while(count <= minTemp){
        thread_pnt[count].reader = fopen(thread_pnt[count].fileOnServer,"r");
        if (thread_pnt[count].reader == NULL){
            perror("the file you want to read to is not possible\n");
            exit(1);
        }
        count++; 
    }

    printf("got to step 1 in combineFiles\n");

    myFile = fopen("resultingFile","a"); 
    if (myFile == NULL){
        perror("The file cannot be written to\n");
        exit(1);
    }   
 
    count = 0;
    minTemp = minimum - 1;
    while(count <= minTemp){
        while( (ch = fgetc(thread_pnt[count].reader)) != EOF){
            fputc(ch,myFile);
        }
        count++; 
    }
    printf("got to step 2 of combineFiles\n"); 
}

/*
 * Creates threads for all the servers based on the minimum value
 */
void createMyThreads(){
    int temporaryMin = minimum - 1;
    int temporaryCounter = 0;
    while(temporaryCounter <= temporaryMin){
        if(pthread_create(&thread_pnt2[temporaryCounter],NULL,readWriteSocket,&thread_pnt[temporaryCounter])){
            perror("problem creating a thread\n");
            exit(1);
        } 
        temporaryCounter++;
    }
   
    temporaryCounter = 0; 
    while(temporaryCounter <= temporaryMin){
        if(pthread_join(thread_pnt2[temporaryCounter],NULL)){
            perror("problem joining first thread\n");
            exit(1);
        } 
        temporaryCounter++;
    }
}

/*
 * This is the main for this class which executes the
 * methods of the client
 */
int
main(int argc, char **argv)
{

    pthread_t thread0, thread1, thread2; 
   
    numArgs(argc);
    checkServerFile(argv[1]);
    getMinAndSetStruct(argv[2], argv[1], argv[3]);
    getFileSize(argv[3]);
    setAvgEtc();
    //printf("end of main, size of file read is: %d \n",fileSize);
    //int sockfd = createSocket();

    //createServerHelper();
    //createServerHelper();
    //createServerHelper();

    //int sockfd = createSocket();
    //createServer(argv[1],argv[2]);
    //connectSocket(sockfd);
    createMyThreads();
    combineFiles();  
  
    return 0;
}
