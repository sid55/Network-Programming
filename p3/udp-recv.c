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
                    if (fileRead == NULL){
                       sprintf(sendBuff,"errorFile");
                    }else{
                       /*
                        * Size of file being read set in global variable
                        */
                        fseek(fileRead,0L,SEEK_END);
                        int fileSize = ftell(fileRead);
                        rewind(fileRead); 
                        sprintf(sendBuff,"infoFile portNum:%d ACK:%d fileSize:",randPort,ACK);
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
                else{
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
