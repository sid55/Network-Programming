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

#define BUFSIZE 2048
#define MAXLINE2 4096 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int     listenfd, connfd,read_size; //the listen and accept file desciptors
struct sockaddr_in servaddr,remaddr; //the server address
uint32_t addrlen = sizeof(remaddr); //length of addresses
char    recvBuff[MAXLINE2]; //the buffer which reads lines
char    sendBuff[MAXLINE2]; //buffer which sends lines
time_t ticks; //ticks variable 
int port; //server port number
int recvlen;//# bytes received 
int msgcnt = 0;			/* count # of messages we received */
pid_t pid;
struct timeval tv;

FILE *fileRead; 
FILE *fileRead2; 


/*
 * This is my server class which performs UDP services. It also performs concurrency
 * by forking itself when a connection has been asked to make to it.
 *
 * NOTE: These are multiple commented out code blocks where I have specified
 * 	 that they are debugging blocks. You may uncomment them to see more
 * 	 information as to what is going on during this communication
 */



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
 * This method creates the UDP socket on the server
 */
int createSocket(){
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		exit(1);
	}else{
		return fd;
	}
}

/*
 * This method initializes the characteristics of the 
 * server and gives it the port number specified by the
 * user.  
 */
void createServer(const char *portnum, int fd){
    sscanf(portnum,"%d",&port); //converts portnum to int
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port); //sets the port number here

    if (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	perror("bind failed");
	exit(1);
    }

}

/*
 * This method basically does the main part of the server. It first waits for a recvfrom and 
 * when gettting a receiveFrom it then forks so that further communication with the client
 * is with the server child. Inside the server child a new socket is created and the parent's one 
 * is closed. The child then does the processing required depending on what instruction the client
 * sends to the server.
 */
void doServer(int fd){

FILE *fileRead;
int ACK = 0;
int setForExit = 0; 

for (;;) {
        bzero(recvBuff, MAXLINE2);
        
		recvlen = recvfrom(fd, recvBuff, MAXLINE2, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
		    pid = fork();
		    if (pid == 0){
			int bytesToRead;
			int position; 

			//parse through recvBuff to get instruction 
			char fileNameT[MAXLINE2]; char seqNumT[MAXLINE2]; char instructR[MAXLINE2];
			bzero(fileNameT,MAXLINE2);
			bzero(seqNumT,MAXLINE2);
			bzero(instructR, MAXLINE2);
			char * pch;
			pch = strtok(recvBuff," ");
			strcpy(instructR,pch);

			if (strncmp(instructR,"WantConnection",14) == 0){

			    //parse through rest of recvBuff
			    pch = strtok(NULL, " ");
			    strcpy(seqNumT,pch);
			    pch = strtok(NULL, " ");
			    strcpy(fileNameT,pch);

			    char fileNameR[MAXLINE2]; char seqNumR[MAXLINE2];
			    bzero(fileNameR,MAXLINE2);
			    bzero(seqNumR,MAXLINE2);
			    memcpy(fileNameR, &fileNameT[9], strlen(fileNameT));
			    memcpy(seqNumR, &seqNumT[7], strlen(seqNumT));
		
			    //Debugging Info	
			    //printf("instruction Real: %s\n",instructR);
			    //printf("filename Real: %s\n",fileNameR);
			    //printf("seqNum Real: %s\n",seqNumR);

			    ACK = atoi(seqNumR);

			    int tempFd = fd;

			    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			    	perror("cannot create socket\n");
			    	exit(1);
			    }
		    
			    //Debugging Info -> shows new socket num in comparison to old one
			    //printf("after create => fd:%d tempFd:%d\n",fd,tempFd);
			    close(tempFd);
	      
			    //make random port
			    int randPort;
			    while(1){
				time_t x;
				srand((unsigned) time(&x));
				//srand(getpid())
				randPort = rand() % (65535 + 1 - 40000) + 40000; 

				bzero(&servaddr, sizeof(servaddr));
				servaddr.sin_family = AF_INET;
				servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
				servaddr.sin_port = htons(randPort); //sets the port number here 

				if (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
				    continue; 
				}else{
				    break;
				}
			    }

			    //Get file size of size wanting
			    bzero(recvBuff, MAXLINE2);
			    bzero(sendBuff, MAXLINE2);
			    fileRead = fopen(fileNameR,"r");
			    fileRead2 = fopen(fileNameR,"r");
			    if (fileRead == NULL){
			       sprintf(sendBuff,"errorFile");
			    }else{
			       /*
				* Size of file being read set in global variable
				*/
				fseek(fileRead,0L,SEEK_END);
				int fileSize = ftell(fileRead);
				rewind(fileRead); 
				sprintf(sendBuff,"infoFile ACK:%d portNum:%d fileSize:",ACK,randPort);
				sprintf(sendBuff + strlen(sendBuff),"%d",fileSize);
				fclose(fileRead);
			    }


			    if (sendto(fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
				perror("sendto");
			    bzero(sendBuff, MAXLINE2);
			    bzero(recvBuff, MAXLINE2);
			    bzero(fileNameR, MAXLINE2);
			    bzero(seqNumR, MAXLINE2);
			    bzero(fileNameT, MAXLINE2);
			    bzero(seqNumT, MAXLINE2);
			    bzero(instructR, MAXLINE2);  
			}//close if statement checking instrcution
			else if (strncmp(instructR,"NumBytesAndPosition",19) == 0){
		   
			    char positionT[MAXLINE2]; char avgBytesT[MAXLINE2];
	 
			    //parse through rest of recvBuff
			    pch = strtok(NULL, " ");
			    strcpy(seqNumT,pch);
			    pch = strtok(NULL, " ");
			    strcpy(positionT,pch);
			    pch = strtok(NULL, " ");
			    strcpy(avgBytesT,pch);
			    
			    char positionR[MAXLINE2]; char seqNumR[MAXLINE2]; char avgBytesR[MAXLINE2];
			    bzero(positionR,MAXLINE2);
			    bzero(seqNumR,MAXLINE2);
			    bzero(avgBytesR,MAXLINE2);
			    memcpy(positionR, &positionT[9], strlen(positionT));
			    memcpy(avgBytesR, &avgBytesT[6], strlen(avgBytesT));
			    memcpy(seqNumR, &seqNumT[7], strlen(seqNumT));
		       
			    //Debugging Info
			    //printf("avgBytes Real: %s",avgBytesR);
			    //printf(" position Real: %s",positionR);
			    //printf(" seqNum Real: %s\n",seqNumR);

			    ACK = atoi(seqNumR);
			    int positionReal = atoi(positionR);
			    int avgBytesReal = atoi(avgBytesR);
			    
			    //Debugging Info
			    //printf("seqNum: %d",ACK);
			    //printf(" position: %d",positionReal);
			    //printf(" avgBytes: %d\n",avgBytesReal);

			    bytesToRead = avgBytesReal;
			    position = positionReal; 

			    int bytesDivisible = 0;
			    int bytesRemainder = 0;    

			    bytesDivisible = bytesToRead/(MAXLINE2 - 14);
			    bytesRemainder = bytesToRead%(MAXLINE2 - 14);
			

			    bzero(sendBuff,MAXLINE2);
			    sprintf(sendBuff, "gotPositionAvg ACK:%d bytesDivisible:%d bytesRemainder:%d servBuffLen:%d servPosition:%d", ACK,bytesDivisible,bytesRemainder,MAXLINE2-14,position);
			    if (sendto(fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
				perror("sendto");


			}else if (strncmp(instructR,"getFileContent",14) == 0){

			    char positionT[MAXLINE2]; char avgBytesT[MAXLINE2];
	 
			    //parse through rest of recvBuff
			    pch = strtok(NULL, " ");
			    strcpy(seqNumT,pch);
			    pch = strtok(NULL, " ");
			    strcpy(positionT,pch);
			    pch = strtok(NULL, " ");
			    strcpy(avgBytesT,pch);
			    
			    char positionR[MAXLINE2]; char seqNumR[MAXLINE2]; char avgBytesR[MAXLINE2];
			    bzero(positionR,MAXLINE2);
			    bzero(seqNumR,MAXLINE2);
			    bzero(avgBytesR,MAXLINE2);
			    memcpy(positionR, &positionT[9], strlen(positionT));
			    memcpy(avgBytesR, &avgBytesT[6], strlen(avgBytesT));
			    memcpy(seqNumR, &seqNumT[7], strlen(seqNumT));
			
			    //Debugging Info
			    //printf("avgBytes Real: %s",avgBytesR);
			    //printf(" position Real: %s",positionR);
			    //printf(" seqNum Real: %s\n",seqNumR);

			    int serverSeqNum = atoi(seqNumR);
			    int positionReal = atoi(positionR);
			    int avgBytesReal = atoi(avgBytesR);
			
			    //Debugging Info
			    //printf("seqNum Part2: %d",serverSeqNum);
			    //printf(" position Part2: %d",positionReal);
			    //printf(" avgBytes Part2: %d\n",avgBytesReal);

			    bytesToRead = avgBytesReal;
			    position = positionReal; 


			    fseek(fileRead2,position,SEEK_SET);
				   

			    //NEED TO DO: need to detach ack number sent to server and attach it on message back to client

			    
			    //Important debugging info -> print if wanted to
			    /* 
				printf("\n\n\n");
				printf("the bytesToRead is: %d\n",bytesToRead);
				printf("the bytesRemainder is: %d\n",bytesRemainder);
				printf("the bytesDivisible is: %d\n",bytesDivisible);
				printf("the position is: %d\n",position);
			    */

			    bzero(sendBuff,MAXLINE2);
			    sprintf(sendBuff,"ACK:%d",serverSeqNum);
			    int readVal = fread(sendBuff+14,sizeof(char),bytesToRead,fileRead2);
		
			    //Debugging Info
			    //printf("buffSize:%d read#bytes:%d\n",(int)strlen(sendBuff),readVal);
			    //printf("%.*s\n",bytesToRead,30+sendBuff);
	    
			    printf("%s\n",sendBuff);
 
			    int writefd = 0;
			    if ((writefd = sendto(fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr *)&remaddr, addrlen)) < 0){
				perror("sendto");
			    }else{
				bzero(sendBuff,MAXLINE2);
			    }
				 
			    bzero(sendBuff,MAXLINE2);
			    bzero(recvBuff,MAXLINE2);

			}//close getFileContent
			

		    }//close pid==0
		    
		}//close if recvlen>0
    	}//close inifinite for
}//close method



int
main(int argc, char **argv)
{
	printf("Server had started...\n");
	numArgs(argc);
	int fd = createSocket();
  	createServer(argv[1],fd);	
        doServer(fd);
}
