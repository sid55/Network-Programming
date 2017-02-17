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
#include <errno.h>

#define MAXLINE 4096 //size of buffer


/*
 * NOTE: All these global variables are not being used in the method
 *       called by pthread_create. The int array is the only
 * 	 one but a mutex lock is done around it to ensure thread safety
 */
char    recvline[MAXLINE], sendline[1024]; //the recieve and send buffers
struct sockaddr_in servaddr2; //the server address

int minimum; //leave as global->not used with threads
FILE *fp; //pointer to server list file, leave as global->not used with threads

int fileSize = -1; //size of file that server has
int remainderBytes = 0; //set as leftover bytes after dividing file size by minimum.
                        //value is added to only first connection then set to 0
int avgBytes = 0; //set as average number of bytes to send per connection
/*
 * A struct thread that will be used throughout this program to hold info about a thread
 */
typedef struct{
    char recvline[MAXLINE + 1]; //the recieve buffer 
    char sendline[1024]; //the send buffer
    int thread_id; //the thread id
    FILE *reader; //used to create seperate file to put recvbuff into
    FILE *fp; //file pointer to server-info.txt
    int avgBytes; //average bytes that can be read
    int remainderBytes; //the remainder bytes left to read
    int minimum; //the minimum connections specified
    int fileSize; //the file size of the file on the server
    int tempVal; //set and unset when sending position to server
    int setFile; //set and unset when expecting certain output from server
    int counter; //set and unset for calculating the average
    char myFileName[1024]; //the name of the file being created
    char fileOnServer[200]; //the name of the file on the server
    char currentLoc; //notes the current location(last character)
    int position; //position to where it the fseek is supposed 
}Thread;

//Thread pointer created -> pointer to an array of structs
Thread *thread_pnt;

//Pointer to array of pthread_t variables
pthread_t *thread_pnt2;

//Int pointer array used to indicate which connections have been made
int *myIntArray;

//The mutex lock variable
pthread_mutex_t lock;

/*
 * This method checks to make sure that the number of 
 * arguments is not invalid and above 3. It will
 * throw an error otherwise.
 */
void numArgs(int argc){
    if (argc != 4){
        printf("usage: a.out <server-info.txt> <num-connections> <filename>\n");
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
        printf("socket error\n");
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
        sscanf(portnum,"%d",&port);  //port number specified
        bzero(&servaddr2, sizeof(servaddr2));
        servaddr2.sin_family = AF_INET;
        servaddr2.sin_port = htons(port); //port number being set
    
        if (inet_pton(AF_INET, ipaddr, &servaddr2.sin_addr) <= 0){
            printf("inet_pton error\n");
            exit(1);
        }
}

/*
 * Makes sure that the server file list provided exists and 
 * that it can be successfully opened.
 */
void checkServerFile(const char *filename){
    if (strncmp(filename,"server-info.txt",15) != 0){
        printf("The file should be server-info.txt\n");
        exit(EXIT_FAILURE);
    }
    fp = fopen(filename,"r");
    if (fp == NULL){
        printf("The file you are trying to open does not exist\n");
        exit(1);
    }
}

//Passing in the filename and the thread object index
void checkServerFile2(const char *filename, Thread threadInfo){
    if (strncmp(filename,"server-info.txt",15) != 0){
        printf("The file should be server-info.txt\n");
        exit(EXIT_FAILURE);
    }
    threadInfo.fp = fopen(filename,"r");
    if (threadInfo.fp == NULL){
        printf("The file you are trying to open does not exist\n");
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
        printf("Not enough servers to connect to\n");
        exit(1);
    }



/*
 * This piece of code is for checking the number of connections that are possible 
 * betweent the server and client
 */
int connectCounter = 0;
while(1){
    int sockfd = createSocket();
    int recfd;

    //Create the server with its info below
    int port;
    char ipAddr[1024]; //max length of ipaddress - used when opening file
    char portNumbr[1024]; //max length of portNumbr - used when opening file
    if(fscanf(fp,"%s %s",ipAddr,portNumbr) > 0){
        if (strncmp(ipAddr,"localhost",9) == 0){
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr2, sizeof(servaddr2));
            servaddr2.sin_family = AF_INET;
            servaddr2.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, "127.0.0.1", &servaddr2.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }
        }else{   
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr2, sizeof(servaddr2));
            servaddr2.sin_family = AF_INET;
            servaddr2.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, ipAddr, &servaddr2.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }
        }        
    }else{
	break;
    }

    //connect the the server's socket here
    if (connect(sockfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) >= 0){
	connectCounter++;
    }
    close(sockfd);
}

    rewind(fp);

    //checking that the the number of connections possible is less than minimum
    if(connectCounter < minimum){
	minimum = connectCounter;
	minimumTemp = connectCounter;
    }


//created thread_pnt2 struct and creates instances of pthread_t
    thread_pnt2 = (pthread_t*)malloc(minimumTemp*sizeof(pthread_t));
    myIntArray = (int*)calloc(minimumTemp,sizeof(int));


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
            printf("The file should be server-info.txt\n");
            exit(EXIT_FAILURE);
        }
        thread_pnt[count].fp = fopen(filename,"r");
        if (thread_pnt[count].fp == NULL){
            printf("The file you are trying to open does not exist\n");
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
        sprintf(fileName2 + strlen(fileName2), "%d", (count));
        sprintf(thread_pnt[count].fileOnServer,"%s",fileName2); 
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

    //sets the name of the server for each pthread
    count = 0;
    minTemp = minimumTemp -1;
    while(count <= minTemp){
        sprintf(thread_pnt[count].myFileName,"%s",fileOnServer);
        count++;
    }

    //sets the count number for each pthread
    count = 0;
    minTemp = minimumTemp -1;
    while(count <= minTemp){
        thread_pnt[count].currentLoc = 0;
        count++;
    }

    
}

/*
 * This method makes an initial connection with the server and recieves
 * the file size of the file it is looking for. If the server cannot be
 * be connected with it goes to the next server and so on and so forth.
 */
void getFileSize(const char* fileName){

while(1){
    int sockfd = createSocket();
    int recfd;

    //Create the server with its info below
    int port;
    char ipAddr[1024]; //max length of ipaddress - used when opneing file
    char portNumbr[1024]; //max length of portNumbr - used when opening file
    if(fscanf(fp,"%s %s",ipAddr,portNumbr) > 0){
        printf("Get file size connection: %s %s \n",ipAddr,portNumbr);
        if (strncmp(ipAddr,"localhost",9) == 0){
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr2, sizeof(servaddr2));
            servaddr2.sin_family = AF_INET;
            servaddr2.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, "127.0.0.1", &servaddr2.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }
        }else{   
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr2, sizeof(servaddr2));
            servaddr2.sin_family = AF_INET;
            servaddr2.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, ipAddr, &servaddr2.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }
        }        
    }else{
        printf("The file is not able to be opened\n");
        exit(1);
    }


    //connect the the server's socket here
    if (connect(sockfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0){
        printf("connect error\n");
        exit(1);
    }

    //send a message with the filename to the server
    sprintf(sendline,"filename");
    sprintf(sendline + strlen(sendline),"%s",fileName);

    if( send(sockfd , sendline , strlen(sendline) , 0) < 0)
    {
        printf("Send failed\n");
        exit(1);     
    } 

    //infinite while loop waiting for info from server
    int bigExit = 0;
    while( (recfd = recv(sockfd , recvline , MAXLINE , 0)) > 0)
    {

        if(strncmp(recvline,"sizeFile",8) == 0){
            char subBuff[200];
            memcpy(subBuff, &recvline[8], strlen(recvline));
            fileSize = atoi(subBuff);
            avgBytes = fileSize/minimum;
            remainderBytes = fileSize%minimum;
            bzero(recvline,MAXLINE);
            bigExit = 1;
            break; 
        }
        if(strncmp(recvline,"errorFile",9) == 0){
            bzero(recvline,MAXLINE);
            printf("The file is not able to be opened\n");  
            break; 
        }

    
    }

    if(bigExit == 1){
        close(sockfd);
        break;
    }
    close(sockfd);
  }

  //sets back pointer to beginning
  rewind(fp);

}

/*
 * This method actually tries to connect the client with the server
 * by using the connect system call.
 */ 
void connectSocket(int sockfd){      
    if (connect(sockfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) < 0){
        printf("connect error\n");
        exit(1);
    }
}

/*
 * Sets avg, total, and remainder in thread_ptr
 */
void setAvgEtc(){
    int count = 1;
    int minTemp = minimum - 1;

    //subtracted minimum -1 for indexing position at right place
    thread_pnt[0].fileSize = fileSize;
    thread_pnt[0].remainderBytes = remainderBytes;
    thread_pnt[0].avgBytes = avgBytes;

    //added minimum -1 for lost bytes 
    while(count <= minTemp){
        thread_pnt[count].fileSize = fileSize;
        thread_pnt[count].remainderBytes = 0;
        thread_pnt[count].avgBytes = avgBytes;
        count++;
    }

    count = 1;
    minTemp = minimum -1;
    int total = 0;
    thread_pnt[0].position = 0;
    while(count <= minTemp){
	thread_pnt[count].position = thread_pnt[count-1].remainderBytes + thread_pnt[count-1].avgBytes + thread_pnt[count-1].position;
	count++;
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
void *readWriteSocket(void *threadInfoTemp){
    Thread *threadInfo = (Thread *)threadInfoTemp;

    //locks the mutex
    pthread_mutex_lock(&lock);

    int sendfd;
    int recfd; //the file descriptor for recieving messages
    int setBreak = 0; //variable set/unset when wanting to break while loops
    struct sockaddr_in servaddr; //the server address


    int sockfd;

    //Create the server with its info below
    int port;
    char ipAddr[1024]; //max length of ipaddress - used when opneing file
    char portNumbr[1024]; //max length of portNumbr - used when opening file
    int setTrue = 0;
    int indexArray = 0;

while(setTrue == 0){

    int sockfd2 = createSocket();

    bzero(portNumbr,1024);
    bzero(ipAddr,1024);

    if(fscanf(threadInfo->fp,"%s %s",ipAddr,portNumbr) > 0){
        printf("%s %s \n",ipAddr,portNumbr);
        if (strncmp(ipAddr,"localhost",9) == 0){
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }
        }else{   
            sscanf(portNumbr,"%d",&port);  //port number specified
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(port); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, ipAddr, &servaddr.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }
        }        
    }else{
	printf("client not able to make number of connections needed\n");
	exit(1);
    }

    int connectfd;

    //Series of if else statements allowing connection to the server
    //Keeps track of what has been visited in an array
    if (((connectfd = connect(sockfd2, (struct sockaddr *) &servaddr, sizeof(servaddr))) >= 0) && myIntArray[indexArray]==0){
	setTrue = 1;
	myIntArray[indexArray] = 1;
	sockfd = sockfd2;
	break;
    }else if (((connectfd = connect(sockfd2, (struct sockaddr *) &servaddr, sizeof(servaddr))) >= 0) && myIntArray[indexArray]==1){
	close(sockfd2);
	setTrue = 0;
    }else{
	close(sockfd2);
	setTrue = 0;
    }


    indexArray++;
}
    
    //closes mutex here
    pthread_mutex_unlock(&lock);

      //infinite while loop that exits either during an error
      //or when the client sends an "exit" message
      while(1){
            //allows sending a string first to the server asking for
            //the file size of the given file
            bzero(threadInfo->sendline,1024);

	    //series of if else statements expecting different recieved output
	    //from the server
            if (threadInfo->setFile == 0){
                sprintf(threadInfo->sendline,"openfile");
                sprintf(threadInfo->sendline + strlen(threadInfo->sendline),"%s",threadInfo->myFileName);
                threadInfo->setFile = 1; 
            }else if (threadInfo->setFile == 1){
                bzero(threadInfo->sendline,1024);
                sprintf(threadInfo->sendline,"size");
                sprintf(threadInfo->sendline + strlen(threadInfo->sendline), "%d", (threadInfo->avgBytes + threadInfo->remainderBytes));
                threadInfo->setFile = 2;
            }else if (threadInfo->setFile == 2){
                sprintf(threadInfo->sendline,"position");
      		sprintf(threadInfo->sendline + strlen(threadInfo->sendline), "%d", threadInfo->position);
                threadInfo->setFile = 3;
            }

            //sends the message to the server 
            if( send(sockfd , threadInfo->sendline , strlen(threadInfo->sendline) , 0) < 0)
            {
                printf("Send failed\n");
                exit(1);     
            }             
           
            //while loop that ends only when the server is done
            //sending its messages to the client            
            while( (recfd = recv(sockfd , threadInfo->recvline , MAXLINE , 0)) > 0)
            {
            int myLength = strlen(threadInfo->recvline);
            const char *gotSizeofFile = &threadInfo->recvline[myLength-5];
            const char *last_four = &threadInfo->recvline[myLength-4];	    

	    //If need needPos or openFile2 is sent back, the loop is broken
            if(strncmp(threadInfo->recvline,"needPos",7) == 0){
                break;
            }

            if(strncmp(threadInfo->recvline,"openFile2",8) == 0){
                break;
            }

	    //This never happens 
            if (strncmp(last_four,"exit",4) == 0){
                 setBreak = 1;
                 break;
              }

	      //If the setFile is to 3, is does  certain implementation
              if(threadInfo->setFile == 3){
                threadInfo->currentLoc = threadInfo->currentLoc + strlen(threadInfo->recvline);
                fseek(threadInfo->reader, threadInfo->currentLoc,SEEK_SET);
                int val = fwrite(threadInfo->recvline,1,strlen(threadInfo->recvline),threadInfo->reader); 
                bzero(threadInfo->recvline,MAXLINE); //zero out buffer
              }

            }
       
 
            if (setBreak == 1){
                setBreak = 0;
                break;
            }
    
            //if there was a recieving error, it will accounted over here 
            if (recfd < 0){
               printf("recv failed on client\n");
               exit(1); 
            }
            if (recfd == 0){
               break;
            }

            bzero(threadInfo->recvline,MAXLINE); //the recieving buffer is reset/zeroed
        }//close second while loop

  fclose(threadInfo->fp); //fclose comes here
  fclose(threadInfo->reader);
  close(sockfd); //look at above when exit is called. it exits automatically
  return NULL; 
}//close method

/*
 * This method combines all the files after it creates a seperate file
 * for each thread 
 */
void combineFiles(){
    int minTemp = minimum - 1;
    int count = 0;
    FILE *myFile;
    char ch;
    while(count <= minTemp){
        thread_pnt[count].reader = fopen(thread_pnt[count].fileOnServer,"r");
        if (thread_pnt[count].reader == NULL){
            printf("the file you want to read to is not possible\n");
            exit(1);
        }
        count++; 
    }

    myFile = fopen("resultingFile","a"); 
    if (myFile == NULL){
        printf("The file cannot be written to\n");
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
}

/*
 * Creates threads for all the servers based on the minimum value
 */
void createMyThreads(){
    int temporaryMin = minimum - 1;
    int temporaryCounter = 0;
    while(temporaryCounter <= temporaryMin){
        if(pthread_create(&thread_pnt2[temporaryCounter],NULL,readWriteSocket,&thread_pnt[temporaryCounter])){
            printf("problem creating a thread\n");
            exit(1);
        } 
        temporaryCounter++;
    }
   
    temporaryCounter = 0; 
    while(temporaryCounter <= temporaryMin){
        if(pthread_join(thread_pnt2[temporaryCounter],NULL)){
            printf("problem joining first thread\n");
            exit(1);
        } 
        temporaryCounter++;
    }
}

/*
 * This method cleans up by removing the files I have created during this 
 * program's execution
 */
void cleanUp(){
    int temp = 0;
    int minT = minimum - 1;
    while(temp <= minT){
        remove(thread_pnt[temp].fileOnServer);
        temp++;
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
    checkServerFile(argv[1]);
    getMinAndSetStruct(argv[2], argv[1], argv[3]);
    getFileSize(argv[3]);
    setAvgEtc();
    if(pthread_mutex_init(&lock,NULL) != 0){
        printf("can't initialize mutex correctly\n");
        exit(1);
    }
 
    createMyThreads();
    combineFiles();  
    cleanUp();
    printf("Program completed successfully!\n"); 
    return 0;
}
