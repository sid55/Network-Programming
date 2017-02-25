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
#include "port.h"

#define MAXLINE 4096 //size of buffer
#define BUFLEN 2048
#define MSGS 100	/* number of messages to send */


/*
 * NOTE: All these global variables are not being used in the method
 *       called by pthread_create. The int array is the only
 * 	 one but a mutex lock is done around it to ensure thread safety
 */
char    recvline[MAXLINE], sendline[1024]; //the recieve and send buffers
struct sockaddr_in servaddr2, myaddr; //the server address

int minimum; //leave as global->not used with threads
FILE *fp; //pointer to server list file, leave as global->not used with threads

int fileSize = -1; //size of file that server has
int remainderBytes = 0; //set as leftover bytes after dividing file size by minimum.
                        //value is added to only first connection then set to 0
int avgBytes = 0; //set as average number of bytes to send per connection
int seqNum = 0; //sequence number used for checking

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
    int realPortNum; //new port num created due to UDP process 
}Thread;

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
void getMinAndSetStruct(const char *numConnections, const char *filename, const char* fileOnServer, int fd){
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

    //connect the the server's socket here
    //if (connect(sockfd, (struct sockaddr *) &servaddr2, sizeof(servaddr2)) >= 0){
	//connectCounter++;
    //}
    int iter = 1, iter2 = 1;
    int result = 2;
    int recvlen;
    int len = sizeof(servaddr2);
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
        printf("the result is: %d\n", result);
        tv.tv_sec = result;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
        sprintf(sendline, "WantConnection SeqNum:%d FileName:%s", seqNum, fileOnServer);
        if (sendto(fd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr2, len)==-1) {
            perror("sendto");
            exit(1);
        }

        recvlen = recvfrom(fd, recvline, BUFLEN, 0, (struct sockaddr *)&servaddr2, &len);
                if (recvlen >= 0) {
                        //recvline[recvlen] = 0;
                        char fileSizeT[MAXLINE]; char AckT[MAXLINE]; char instructR[MAXLINE]; char portT[MAXLINE];
                        bzero(fileSizeT,MAXLINE);
                        bzero(AckT,MAXLINE);
                        bzero(instructR, MAXLINE);
                        bzero(portT, MAXLINE);
                        char * pch;
                        pch = strtok(recvline," ");
                        strcpy(instructR,pch);
                        if (strncmp(instructR, "errorFile", 9) == 0){
                            printf("Connection success -> errorFile\n");
                            break;
                        }else if (strncmp(instructR, "infoFile", 8) == 0){
                            printf("Connection success: %s\n", recvline);
                            pch = strtok(NULL, " ");
                            strcpy(portT,pch);
                            pch = strtok(NULL, " ");
                            strcpy(AckT,pch);
                            pch = strtok(NULL, " ");
                            strcpy(fileSizeT,pch);

                            char fileSizeR[MAXLINE]; char AckR[MAXLINE]; char portR[MAXLINE];
                            bzero(fileSizeR,MAXLINE);
                            bzero(AckR,MAXLINE);
                            memcpy(fileSizeR, &fileSizeT[9], strlen(fileSizeT));
                            memcpy(AckR, &AckT[4], strlen(AckT));
                            memcpy(portR, &portT[8], strlen(portT));
 
                            //printf("filesize Real: %s\n",fileSizeR);
                            //printf("AckNum Real: %s\n",AckR);
                            //printf("portNum Real: %s\n",portR);

                            //mySeqNumber = atoi(seqNumR);
                            fileSize = atoi(fileSizeR);
                            int ackNum = atoi(AckR);
                            int portReal = atoi(portR);                
                            
                            printf("ackNum:%d portReal:%d fileSize:%d \n",ackNum,portReal,fileSize);       
                            if (seqNum == ackNum){
                                myPortNumArray[portIndex] = portReal;
                                portIndex++;
                                seqNum++; 
                                connectCounter++;
                                break;
                            }else{
                                printf("print here for testing purposes\n");
                            }
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

    count = 0;
    minTemp = minimumTemp - 1;
    while(count <= minTemp){
        thread_pnt[count].realPortNum = myPortNumArray[count];
        count++; 
    }

    //count = 0;
    //minTemp = minimumTemp -1;
    //while(count <= minTemp){
    //    printf("portNum:%d at index:%d \n", thread_pnt[count].realPortNum,count);
    //    count++;
    //}
 
}








int main(int argc, char **argv)
{
	//struct sockaddr_in remaddr;
	int fd; //i , slen=sizeof(remaddr);
	//char buf[BUFLEN];	/* message buffer */
	//int recvlen;		/* # bytes in acknowledgement message */
	//char *server = "127.0.0.1";	/* change this to use a different server */

    numArgs(argc);
    checkServerFile(argv[1]); 
    fd = createSocket();
    createClient(fd);
    getMinAndSetStruct(argv[2],argv[1],argv[3],fd); 
    printf("the minimum is: %d\n", minimum); 

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

    /*
	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(SERVICE_PORT);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
    */


	/* now let's send the messages */

    /*
	for (i=0; i < MSGS; i++) {
		printf("Sending packet %d to %s port %d\n", i, server, SERVICE_PORT);
		sprintf(buf, "This is packet %d", i);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			exit(1);
		}

		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
                if (recvlen >= 0) {
                        buf[recvlen] = 0;	
                        printf("received message: \"%s\"\n", buf);
                }
	}
    */
	close(fd);
	return 0;
}
