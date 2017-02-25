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
#include "port.h"

#define BUFSIZE 2048
#define MAXLINE2 4096 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int     listenfd, connfd,read_size; //the listen and accept file desciptors
struct sockaddr_in servaddr,remaddr; //the server address
socklen_t addrlen = sizeof(remaddr); //length of addresses
char    recvBuff[MAXLINE2]; //the buffer which reads lines
char    sendBuff[MAXLINE2]; //buffer which sends lines
time_t ticks; //ticks variable 
int port; //server port number
int recvlen;//# bytes received 
int msgcnt = 0;			/* count # of messages we received */
pid_t pid;

FILE *fileRead; 
FILE *fileRead2; 

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

void doServer(int fd){

FILE *fileRead;
int ACK = 0;
int bytesToRead;
int position;
int setForExit = 0; 

for (;;) {
        bzero(recvBuff, MAXLINE2);
		recvlen = recvfrom(fd, recvBuff, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
            pid = fork();
            if (pid == 0){
                printf("in child process\n");

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

                    //printf("filename: %s\n",fileNameT);
                    //printf("seqNum: %s\n",seqNumT);
                    
                    char fileNameR[MAXLINE2]; char seqNumR[MAXLINE2];
                    bzero(fileNameR,MAXLINE2);
                    bzero(seqNumR,MAXLINE2);
                    memcpy(fileNameR, &fileNameT[9], strlen(fileNameT));
                    memcpy(seqNumR, &seqNumT[7], strlen(seqNumT));
                
                    //printf("instruction Real: %s\n",instructR);
                    //printf("filename Real: %s\n",fileNameR);
                    //printf("seqNum Real: %s\n",seqNumR);

                    ACK = atoi(seqNumR);



                    //printf("before close fd: %d \n", fd);
                    int tempFd = fd;
                    close(fd);

                    //printf("after close fd: %d \n",fd);
                  
                    //make sure not to use same socket 
                    while(1){ 
                        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                            perror("cannot create socket\n");
                            exit(1);
                        }
                        if (fd != tempFd){
                            printf("plz not here \n");
                            close(fd);
                            continue;
                        }else{
                            break;
                        }
                    }
            
                    //printf("after create socket fd is: %d \n",fd);

      
                    //make random port
                    int randPort;
                    while(1){
                        time_t x;
                        srand((unsigned) time(&x));
                        randPort = rand() % (65535 + 1 - 40000) + 40000; 
                        printf("the randPort is: %d \n",randPort);

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
                    //fclose(fileRead);


                    printf("sending response: %s \n", sendBuff);
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
                    printf("threading complete recieved is: %s\n", recvBuff);
           
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
                
                    //printf("avgBytes Real: %s",avgBytesR);
                    //printf(" position Real: %s",positionR);
                    //printf(" seqNum Real: %s\n",seqNumR);

                    ACK = atoi(seqNumR);
                    int positionReal = atoi(positionR);
                    int avgBytesReal = atoi(avgBytesR);
                    
                    //printf("seqNum: %d",ACK);
                    //printf(" position: %d",positionReal);
                    //printf(" avgBytes: %d\n",avgBytesReal);

                    bytesToRead = avgBytesReal;
                    position = positionReal; 

                    bzero(sendBuff,MAXLINE2);
                    sprintf(sendBuff, "gotPositionAvg ACK:%d", ACK);
                    if (sendto(fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
                        perror("sendto");


                }else if (strncmp(instructR,"getFileContent",14) == 0){

                    
                    pch = strtok(NULL, " ");
                    strcpy(seqNumT,pch);
                    char seqNumR[MAXLINE2]; 
                    bzero(seqNumR,MAXLINE2);
                    memcpy(seqNumR, &seqNumT[7], strlen(seqNumT));
                    int serverSeqNum = atoi(seqNumR);
                    serverSeqNum++;

                    fseek(fileRead2,position,SEEK_SET);
                           
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
                            fseek(fileRead2,position,SEEK_SET);
                            fread(sendBuff,sizeof(char),bytesRemainder,fileRead2);
                            fseek(fileRead2,position + bytesRemainder,SEEK_SET);
                            printf("%s",sendBuff);
                            if (temp == bytesDivisible){
                                setForExit = 1;
                            }
                        }else{
                            fseek(fileRead2,position + bytesRemainder + ((temp - 1) * MAXLINE2),SEEK_SET);
                            fread(sendBuff,sizeof(char),MAXLINE2,fileRead2);
                            printf("%s",sendBuff);
                            if (temp == bytesDivisible){
                                setForExit = 1;
                            }
                        }
                        //if ((writefd = write(connfd, sendBuff, strlen(sendBuff))) > 0) {
                        //    bzero(sendBuff,MAXLINE2);
                        //}
            

                        
                        if (sendto(fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr *)&remaddr, addrlen) < 0){
                            perror("sendto");
                        }else{
                            bzero(sendBuff,MAXLINE2);
                        }
                        


                        
                        /* 
                        //server now acts like client in sending data and sends upto three times to ensure that data
                        //actually gets sent
                        int iter = 1, iter2 = 1;
                        int result = 2;
                        int recvlen;
                        int len = sizeof(servaddr);
                        struct timeval tv;
                        //sends packet out three times to figure out how long it takes
                        while (iter <= 3 ){
                            bzero(recvBuff,MAXLINE2);        
                            bzero(sendBuff,MAXLINE2);        

                            //power function for exponential backoff
                            while(iter2 > 1){
                                result = result * 2;
                                iter2--; 
                            }
                            printf("the timeout in server is: %d\n", result);
                            tv.tv_sec = result;
                            tv.tv_usec = 0;
                            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));
            

                            if (sendto(fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr *)&remaddr, addrlen) < 0){
                                perror("sendto");
                            }else{
                                bzero(sendBuff,MAXLINE2);
                            }

                            
                            recvlen = recvfrom(fd, recvBuff, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
                                    if (recvlen >= 0) {
                                            //recvline[recvlen] = 0;
                                            char AckT[MAXLINE2]; char instructR[MAXLINE2]; 
                                            bzero(AckT,MAXLINE2);
                                            bzero(instructR, MAXLINE2);
                                            char * pch;
                                            pch = strtok(recvBuff," ");
                                            strcpy(instructR,pch);

                                            char AckR[MAXLINE2];
                                            pch = strtok(NULL, " ");
                                            strcpy(AckT,pch);
                                            bzero(AckR,MAXLINE2);
                                            memcpy(AckR, &AckT[4], strlen(AckT));
                                            int ackNum = atoi(AckR);

                                            if(serverSeqNum == ackNum){
                                                if (strncmp(instructR, "gotData", 7) == 0){
                                                    serverSeqNum++; 
                                                    break;
                                                }
                                            }else{
                                                printf("testing purposes => packet loss => retransmit\n");
                                            }
                                    }
                        iter++;
                        iter2 = iter;
                        result = 2;
                        }//close iteration while loop
                        */












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
                       
                     fclose(fileRead2); //fclose here

                     bzero(sendBuff,MAXLINE2);
                     bzero(recvBuff,MAXLINE2);




                    //bzero(sendBuff, MAXLINE2);
                    //sprintf(sendBuff,"fileContent ACK:%d",ACK);
                    //if (sendto(fd, sendBuff, strlen(sendBuff), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
                    //    perror("sendto");

                }else{
                    printf("Not supposed to come here yet(other instructions exist\n");
                    close(fd);
                }

            }else{
                printf("in parent process\n");
            }
		}
		else if(recvlen == 0){
            printf("Awesome\n");
        }else{
			printf("something went wrong!\n");
        }
    }
}



int
main(int argc, char **argv)
{
	//unsigned char buf[BUFSIZE];	/* receive buffer */
    
	numArgs(argc);
	int fd = createSocket();
  	createServer(argv[1],fd);	
    doServer(fd);
	
}
