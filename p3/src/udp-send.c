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
#define BUFLEN 4096 
#define MSGS 100	/* number of messages to send */


/*
 * NOTE: All these global variables are not being used in the method
 *       called by pthread_create. The int array is the only
 * 	 one but a mutex lock is done around it to ensure thread safety
 *
 * NOTE: I have left little debug statements throughout the code for
 * 	 observational purposes. You may uncomment those pieces of 
 * 	 code to further see in detail of what is going on.
 */
char    recvline[MAXLINE], sendline[1024]; //the recieve and send buffers
struct sockaddr_in servaddr2, myaddr; //the server address

int minimum; //leave as global->not used with threads
FILE *fp; //pointer to server list file, leave as global->not used with threads

int fileSize = -1; //size of file that server has
int remainderBytes = 0; //set as leftover bytes after dividing file size by minimum.
                        //value is added to only first connection then set to 0
int avgBytes = 0; //set as average number of bytes to send per connection
//int seqNum = 0; //sequence number used for checking

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
    int currentLoc; //notes the current location(last character)
    int position; //position to where it the fseek is supposed
    int realPortNum; //new port num created due to UDP process
    char realIPaddr[1024]; //ip address for new port above
}Thread;

/*
 * A struct for just holding the ip address at a given time
 */
typedef struct{
    char myIPaddr[1024];
}IPaddr;

//IPaddr pointer created -> pointer to ipaddr struct above
IPaddr *ipPointer;

//Thread pointer created -> pointer to an array of structs
Thread *thread_pnt;

//Pointer to array of pthread_t variables
pthread_t *thread_pnt2;

//Int pointer array used to indicate which connections have been made
int *myIntArray;

//Port number array to know which port numbers to connect to after
int *myPortNumArray;

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
    int sockfdtemp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfdtemp < 0){
        printf("socket error\n");
        exit(1);
    }
    return sockfdtemp;
}

/*
 * This method sets up the client's struct and additionaly binds 
 * the struct to the client's socket
 */
void createClient(int fd){
    memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);
    /*
	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
        exit(1);
	} 
    */
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
    fp = fopen(filename,"r");
    if (fp == NULL){
        printf("The file you are trying to open does not exist\n");
        exit(1);
    }
}

//Passing in the filename and the thread object index
void checkServerFile2(const char *filename, Thread threadInfo){
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
void getMinAndSetStruct(const char *numConnections, const char *filename, const char* fileOnServer, int fd){
    int tempNumConnections = -1;
    int tempNumLines = 0;
    int ch = 0;
    int seqNum = 0; //sequence number used for checking

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

    ipPointer = (IPaddr*)malloc(minimumTemp*sizeof(IPaddr));
    myPortNumArray = (int*)calloc(minimumTemp,sizeof(int));
    int portIndex = 0;

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


    /*
     * This brief piece of code checks how many servers out there have the file
     * that the client needs and how many servers are even active from the 
     * server file list
     */
    int iter = 1, iter2 = 1;
    int result = 2;
    int recvlen;
    uint32_t len = sizeof(servaddr2);
    struct timeval tv;
    //sends packet out three times to figure out how long it takes
    while (iter <= 3 ){
        bzero(recvline,MAXLINE);        
        bzero(sendline,1024);        

        //power function for exponential backoff
        while(iter2 > 1){
            result = result * 2;
            iter2--; 
        }
        sprintf(sendline, "WantConnection SeqNum:%d FileName:%s", seqNum, fileOnServer);
        if (sendto(fd, sendline, MAXLINE /*strlen(sendline)*/, 0, (struct sockaddr *)&servaddr2, len)<0) {
            perror("sendto");
            exit(1);
        }

	tv.tv_sec = result;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
        recvlen = recvfrom(fd, recvline, BUFLEN, 0, (struct sockaddr *)&servaddr2, &len);
                if (recvlen >= 0) {
                        char fileSizeT[MAXLINE]; char AckT[MAXLINE]; char instructR[MAXLINE]; char portT[MAXLINE];
                        bzero(fileSizeT,MAXLINE);
                        bzero(AckT,MAXLINE);
                        bzero(instructR, MAXLINE);
                        bzero(portT, MAXLINE);
                        char * pch;
                        pch = strtok(recvline," ");
                        strcpy(instructR,pch);

                        char AckR[MAXLINE];
                        pch = strtok(NULL, " ");
                        strcpy(AckT,pch);
                        bzero(AckR,MAXLINE);
                        memcpy(AckR, &AckT[4], strlen(AckT));
                        int ackNum = atoi(AckR);
				
			//Debugging Info
			//printf("the seqNum:%d ackNum:%d \n",seqNum,ackNum);

                        if(seqNum == ackNum){
                            if (strncmp(instructR, "errorFile", 9) == 0){
                                break;
                            }else if (strncmp(instructR, "infoFile", 8) == 0){
                                pch = strtok(NULL, " ");
                                strcpy(portT,pch);
                                pch = strtok(NULL, " ");
                                strcpy(fileSizeT,pch);

                                char fileSizeR[MAXLINE]; char portR[MAXLINE];
                                bzero(fileSizeR,MAXLINE);
                                memcpy(fileSizeR, &fileSizeT[9], strlen(fileSizeT));
                                memcpy(portR, &portT[8], strlen(portT));
    
				//Debgging Info 
                                //printf("filesize Real: %s\n",fileSizeR);
                                //printf("AckNum Real: %s\n",AckR);
                                //printf("portNum Real: %s\n",portR);
                                //mySeqNumber = atoi(seqNumR);

                                fileSize = atoi(fileSizeR);
                                int portReal = atoi(portR);                
                                
				//Debugging Info
                                //printf("ackNum:%d portReal:%d fileSize:%d ipaddr:%s\n",ackNum,portReal,fileSize,ipAddr);       
      
                                myPortNumArray[portIndex] = portReal;
                                sprintf(ipPointer[portIndex].myIPaddr,"%s",ipAddr);
                                portIndex++;
                                seqNum++; 
                                connectCounter++;
                                break;
                            }
                        }else{
                            printf("packet loss => retransmit\n");
                        }
                }
    iter++;
    iter2 = iter;
    result = 2;
    }//close iteration while loop

 
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

    //error check for 0 servers open/ no servers have file
    if (minimumTemp == 0){
        printf("Either no servers are online or no servers have the file\n");
        exit(1);
    }

    avgBytes = fileSize/minimum;
    remainderBytes = fileSize%minimum;


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
        sprintf(fileName2,"%s-",fileOnServer);
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

    //sets ip addr and port of available ports and ipaddr
    count = 0;
    minTemp = minimumTemp - 1;
    while(count <= minTemp){
        thread_pnt[count].realPortNum = myPortNumArray[count];
        sprintf(thread_pnt[count].realIPaddr,"%s",ipPointer[count].myIPaddr);
        count++; 
    }

    //Debugging information
    /*
    count = 0;
    minTemp = minimumTemp -1;
    while(count <= minTemp){
        printf("portNum:%d at index:%d \n", thread_pnt[count].realPortNum,count);
        printf("ipAddr:%s at index:%d \n",thread_pnt[count].realIPaddr,count);
        count++;
    }
    */
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
 * The main logic of this program is in this method. The client 
 * communicates with the server using the sendTo and recvFrom system 
 * calls. It has two buffers, one for sending messages and one
 * for recieving messages. This entire method is put under a while
 * loop to allow the client to continuously send messages to the
 * server and there is a seperate while loop inside this client
 * for receiving messages in case the message being sent by the server
 * will require multiple packets. Other while loops are for packet 
 * retransmission if packets are lost. If there are any problems using
 * those two system calls, errors are thrown respectively.
 *
 */
void *readWriteSocket(void *threadInfoTemp){
    Thread *threadInfo = (Thread *)threadInfoTemp;

    int fd = createSocket();

    struct sockaddr_in servaddr; //the server address

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(threadInfo->realPortNum); //port number being set

    //ip address specified below by the user
    if (strncmp(threadInfo->realIPaddr,"localhost",9) == 0){  
        if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0){
            printf("inet_pton error\n");
            exit(1);
        }
    }else{
        if (inet_pton(AF_INET, threadInfo->realIPaddr, &servaddr.sin_addr) <= 0){
            printf("inet_pton error\n");
            exit(1);
        }
    }
   
    //Continue the communication between the pthread and the forked server. In here the position
    //and the number of bytes is sent to the server 
    int iter = 1, iter2 = 1;
    int result = 2;
    int recvlen;
    int len = sizeof(servaddr);
    struct timeval tv;
    int seqNum = 5;

    int servBytesDivisible;
    int servBuffLen;
    int servBytesRemainder;
    int servPosition;
    int temp = 0;
    int setClientExit = 0;
    
    pthread_mutex_lock(&lock);
    //sends packet out three times to figure out how long it takes
    while (iter <= 3 ){
        bzero(threadInfo->recvline,MAXLINE);        
        bzero(threadInfo->sendline,1024);        

        //power function for exponential backoff
        while(iter2 > 1){
            result = result * 2;
            iter2--; 
        }
        tv.tv_sec = result;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
        sprintf(threadInfo->sendline, "NumBytesAndPosition SeqNum:%d Position:%d Bytes:%d", seqNum, threadInfo->position, (threadInfo->avgBytes + threadInfo->remainderBytes));
        if (sendto(fd, threadInfo->sendline, strlen(threadInfo->sendline), 0, (struct sockaddr *)&servaddr, len)<0) {
            perror("sendto");
            exit(1);
        }

	bzero(threadInfo->recvline,MAXLINE); 
        recvlen = recvfrom(fd, threadInfo->recvline, BUFLEN, 0, (struct sockaddr *)&servaddr, &len);
                if (recvlen >= 0) {
                        //recvline[recvlen] = 0;
                        char AckT[MAXLINE]; char instructR[MAXLINE]; 
                        bzero(AckT,MAXLINE);
                        bzero(instructR, MAXLINE);
                        char * pch;
                        pch = strtok(threadInfo->recvline," ");
                        strcpy(instructR,pch);

                        char AckR[MAXLINE];
                        pch = strtok(NULL, " ");
                        strcpy(AckT,pch);
                        bzero(AckR,MAXLINE);
                        memcpy(AckR, &AckT[4], strlen(AckT));
                        int ackNum = atoi(AckR);
                        if(seqNum == ackNum){
                            if (strncmp(instructR, "gotPositionAvg", 14) == 0){
                                char bytesDivisibleT[MAXLINE]; char bytesRemainderT[MAXLINE]; char servBuffLenT[MAXLINE];
                                char servPositionT[MAXLINE]; 
                                bzero(bytesDivisibleT,MAXLINE);
                                bzero(bytesRemainderT, MAXLINE);
                                bzero(servBuffLenT, MAXLINE);
                                bzero(servPositionT,MAXLINE);

                                char bytesDivisibleR[MAXLINE];
                                pch = strtok(NULL, " ");
                                strcpy(bytesDivisibleT,pch);
                                bzero(bytesDivisibleR,MAXLINE);
                                memcpy(bytesDivisibleR, &bytesDivisibleT[15], strlen(bytesDivisibleT));
                                servBytesDivisible = atoi(bytesDivisibleR);
				

                                char bytesRemainderR[MAXLINE];
                                pch = strtok(NULL, " ");
                                strcpy(bytesRemainderT,pch);
                                bzero(bytesRemainderR,MAXLINE);
                                memcpy(bytesRemainderR, &bytesRemainderT[15], strlen(bytesRemainderT));
                                servBytesRemainder = atoi(bytesRemainderR);

                                char servBuffLenR[MAXLINE];
                                pch = strtok(NULL, " ");
                                strcpy(servBuffLenT,pch);
                                bzero(servBuffLenR,MAXLINE);
                                memcpy(servBuffLenR, &servBuffLenT[12], strlen(servBuffLenT));
                                servBuffLen = atoi(servBuffLenR);

                                char servPositionR[MAXLINE];
                                pch = strtok(NULL, " ");
                                strcpy(servPositionT,pch);
                                bzero(servPositionR,MAXLINE);
                                memcpy(servPositionR, &servPositionT[13], strlen(servPositionT));
                                servPosition = atoi(servPositionR);

                                
				//Debugging Info
                                //printf("thread_id:%d bytesRemain:%d bytesDivis:%d servBuffLen:%d servPosition:%d\n",threadInfo->thread_id,servBytesRemainder,servBytesDivisible,servBuffLen,servPosition);
 
 
                                seqNum++; 
                                break;
                            }
                        }else{
                            printf("packet loss => retransmit\n");
                        }
		}
    iter++;
    iter2 = iter;
    result = 2;
    }//close iteration while loop

    pthread_mutex_unlock(&lock);

    if(iter == 4){
	printf("Server(s) is not able to send back information to you\n");
	exit(1);
    }
 
    //In here the client recieves the entire portion of the file it is expecting to recieve
    iter = 1; iter2 = 1;
    result = 2;
    len = sizeof(servaddr);
    seqNum = 1000000000;
    //sends packet out three times to figure out how long it takes
    while (iter <= 3 ){
        bzero(threadInfo->recvline,MAXLINE);        
        bzero(threadInfo->sendline,1024);        

        //power function for exponential backoff
        while(iter2 > 1){
            result = result * 2;
            iter2--; 
        }
        tv.tv_sec = result;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
        bzero(threadInfo->sendline,1024);

        //debugging info
        /*
        printf("seqNum is: %d\n",seqNum);
        printf("servPos is: %d\n", servPosition);
        printf("servBytesRemain is: %d\n",servBytesRemainder);
	*/
        sprintf(threadInfo->sendline, "getFileContent SeqNum:%d Position:%d Bytes:%d", seqNum, servPosition, servBytesRemainder);
	
        if (sendto(fd, threadInfo->sendline, strlen(threadInfo->sendline), 0, (struct sockaddr *)&servaddr, len)<0) {
            perror("sendto");
            exit(1);
        }
       
	int superExit = 0; 
        recvlen = recvfrom(fd, threadInfo->recvline, BUFLEN, 0, (struct sockaddr *)&servaddr, &len);
                if (recvlen >= 0) {

                        //NEED TO DO -> Ack confirmation on client side and increase seqNum by 1






                        int temp = 0;
			int innerRetransmit = 2;
			int innerIteration = 1;

			while(temp <= servBytesDivisible){

				while(innerIteration <= 3){


				   tv.tv_sec = innerRetransmit;
				   tv.tv_usec = 0;
				   setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));


				    bzero(threadInfo->sendline,1024);
				    if(temp == 0){
					sprintf(threadInfo->sendline, "getFileContent SeqNum:%d Position:%d Bytes:%d", seqNum, servPosition, servBytesRemainder); 
					if (sendto(fd, threadInfo->sendline, strlen(threadInfo->sendline), 0, (struct sockaddr *)&servaddr, len)<0) {
					    perror("sendto");
					    exit(1);
					}

					if (temp == servBytesDivisible){
					    setClientExit = 1;
					}
				    }else{
					sprintf(threadInfo->sendline, "getFileContent SeqNum:%d Position:%d Bytes:%d", seqNum, servPosition + servBytesRemainder + ((temp-1)*servBuffLen), servBuffLen); 
					if (sendto(fd, threadInfo->sendline, strlen(threadInfo->sendline), 0, (struct sockaddr *)&servaddr, len)<0) {
					    perror("sendto");
					    exit(1);
					}

					if (temp == servBytesDivisible){
					    setClientExit = 1;
					}
				    }

				    recvlen = recvfrom(fd, threadInfo->recvline, BUFLEN, 0, (struct sockaddr *)&servaddr, &len);
				    if (recvlen>0){
					    char myAckArray[MAXLINE]; char restBuffer[MAXLINE];
					    bzero(myAckArray,MAXLINE);
					    bzero(restBuffer,MAXLINE);

					    int length = (recvlen - 14);
					    strncpy(myAckArray,threadInfo->recvline + 0,14);

					    char myAckArrayReal[MAXLINE];
					    bzero(myAckArrayReal,MAXLINE);
					    strncpy(myAckArrayReal,myAckArray + 4,10);

					    int tempooSeqNumber = atoi(myAckArrayReal);
					    if (tempooSeqNumber == seqNum){
						    strncpy(restBuffer,threadInfo->recvline + 14,(int)strlen(threadInfo->recvline));
						    threadInfo->currentLoc = (threadInfo->currentLoc + length)/*(int)strlen(threadInfo->recvline)*/;
						    fseek(threadInfo->reader, threadInfo->currentLoc,SEEK_SET);
						    int val = fwrite(restBuffer,1,length,threadInfo->reader); 
						
						    //Debugging Info
						    //printf("thread_id:%d currentLoc:%d FWriteVal:%d recvlen:%d strelen:%d\n",threadInfo->thread_id,threadInfo->currentLoc,val,recvlen,(int)strlen(threadInfo->recvline));

						    bzero(threadInfo->recvline,MAXLINE); //zero out buff
						    seqNum++;
						    break;
					    }else{
						   printf("packet loss => retransmit\n");
					    }
				    }else if (recvlen == 0){
					superExit = 1;
					break;
				    }else{
					printf("packet loss => retransmit\n");	
				    }

				innerIteration++;
				innerRetransmit = innerRetransmit * 2;
				}//close retransmit loop here

			    if(innerIteration == 4){
				printf("The server cannot be contacted\n");
				cleanUp();
				exit(1);
			    }	

			    if(superExit == 1){
			   	break;
			    }

                            bzero(threadInfo->sendline,1024);
                            temp++;
                        }

                        break;
                }
    iter++;
    iter2 = iter;
    result = 2;
    }//close iteration while loop

    if(iter==4){
	printf("Server(s) is not able to send back information to you\n");
	exit(1);
    }

 
    printf("Thread %d has finished collecting data\n",threadInfo->thread_id); 
    close(fd);
}


/*
 * This method combines all the files after it creates a seperate file
 * for each thread 
 */
void combineFiles(const char* fileOnServer){
    int minTemp = minimum - 1;
    int count = 0;
    FILE *myFile;
    char ch;
    char fileNewBuff[200];
    bzero(fileNewBuff,200);
    while(count <= minTemp){
        rewind(thread_pnt[count].reader);
        thread_pnt[count].reader = fopen(thread_pnt[count].fileOnServer,"r");
        if (thread_pnt[count].reader == NULL){
            printf("the file you want to read to is not possible\n");
            exit(1);
        }
        count++; 
    }

    sprintf(fileNewBuff,"%s.out",fileOnServer);
    if (access(fileNewBuff,F_OK)!=-1){
        remove(fileNewBuff);
    }

    myFile = fopen(fileNewBuff,"a"); 
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






int main(int argc, char **argv)
{
    printf("Communication has Started\n");
    int fd;
    numArgs(argc);
    checkServerFile(argv[1]); 
    fd = createSocket();
    createClient(fd);
    printf("Checking which connections are active... (This could take some time)\n");
    getMinAndSetStruct(argv[2],argv[1],argv[3],fd); 
    setAvgEtc();
    if(pthread_mutex_init(&lock,NULL) != 0){
        printf("can't initialize mutex correctly\n");
        exit(1);
    }

    printf("Starting to recieve data from server and place into file\n"); 
    createMyThreads();
    combineFiles(argv[3]);
    cleanUp();
    close(fd);
    printf("\nClient has finished collecting data from servers\n");
    return 0;
}
