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
struct sockaddr_in servaddr2, servaddr3,myaddr; //the server address

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

int main(){

	    int fd = socket(AF_INET, SOCK_DGRAM, 0);
	    if (fd < 0){
		printf("socket error\n");
		exit(1);
	    }


	    bzero(&servaddr2, sizeof(servaddr2));
            servaddr2.sin_family = AF_INET;
            servaddr2.sin_port = htons(48931); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, "127.0.0.1", &servaddr2.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }


	uint32_t len = sizeof(servaddr2);


	sprintf(sendline, "hello");
        if (sendto(fd, sendline, MAXLINE /*strlen(sendline)*/, 0, (struct sockaddr *)&servaddr2, len)<0) {
            perror("sendto");
            exit(1);
        }
	close(fd);



	
	 fd = socket(AF_INET, SOCK_DGRAM, 0);
	    if (fd < 0){
		printf("socket error\n");
		exit(1);
	    }


	len = sizeof(servaddr3);

       	bzero(&servaddr3, sizeof(servaddr3));
            servaddr3.sin_family = AF_INET;
            servaddr3.sin_port = htons(49032); //port number being set
    
            //ip address specified below by the user  
            if (inet_pton(AF_INET, "127.0.0.1", &servaddr3.sin_addr) <= 0){
                printf("inet_pton error\n");
                exit(1);
            }

	sprintf(sendline, "hello");
        if (sendto(fd, sendline, MAXLINE /*strlen(sendline)*/, 0, (struct sockaddr *)&servaddr3, len)<0) {
            perror("sendto");
            exit(1);
        }

	close(fd);


	return 0;
}
