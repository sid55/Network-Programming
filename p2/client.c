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

int     sockfd, n; //the socket file descriptor
char    recvline[MAXLINE + 1], sendline[1024]; //the recieve and send buffers
struct sockaddr_in servaddr; //the server address
int port; //the server port number

FILE *fp; //pointer to server list file
FILE *fileRead; //pointer to file that has book
char line [1024]; //assumption line length of 1024 bytes
size_t len = 0;
ssize_t readfp;
char ipAddr[1024]; //max length of ipaddress - used when opneing file
char portNumbr[1024]; //max length of portNumbr - used when opening file
int fileSize = -1; //size of file that has book
int minimum = -1; //the minimum num of servers depending on file and user input
int counter = 0; //set after first connection, used to calc position to set fseek on server
int remainderBytes = 0; //set as leftover bytes after dividing file size by minimum.
                        //value is added to only first connection then set to 0
int avgBytes = 0; //set as average number of bytes to send per connection
int setBreak = 0; //variable set/unset when wanting to break while loops
char *threadArrayPointer;
int tempVal = 0; //used for finidng position initially
/*
 * A struct thread that will be used throughout this program to hold info about a thread
 */
typedef struct{
    int thread_id;
    FILE *reader;
    char ipAddr[1024];
    char portNumbr[1024];
    int sockID;
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
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("socket error\n");
        exit(1);
    }
    return sockfd;
}

/*
 * This method gives the server information such as the port
 * number specified by the user and the ip address. It allows
 * the client to know information about the server so that the
 * client can communicate with the server.
 */
void createServer(const char *ipaddr, const char *portnum){
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
void createServerHelper(){
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

    //Set the minimum to one of those values
    if(tempNumLines <= tempNumConnections){
        minimum = tempNumLines;
    }else{
        minimum = tempNumConnections;
    }

    thread_pnt = (Thread*)malloc(minimum*sizeof(Thread)); 
    //thread_pnt[1].thread_id = 5;

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
void readWriteSocket(int sockfd, const char* fileName){
  int recfd; //the file descriptor for recieving messages
  int setFileBytes = 0; //set and unset when wanting to know file size from server
                        //only set and unset once for first connection
  int setSendBytes = 0; //set and unset when wanting to send the number of bytes
                        //that the client wants the server to read from the file

  while(minimum > 0){
    createServerHelper();
    connectSocket(sockfd); 
      //infinite while loop that exits either during an error
      //or when the client sends an "exit" message
      while(1){
            bzero(sendline,1024); //zeroing out the buffer
            
            //allows sending a string first to the server asking for
            //the file size of the given file
            if(setFileBytes == 0){
                printf("GOT INTO SETFILEBYTES ONE\n");
                sprintf(sendline,fileName);
                setSendBytes = 1;
            }else if(setSendBytes == 1){
                printf("GOT INTO SETSENDBYTES ONE\n");
                sprintf(sendline, "%d", (long) (avgBytes + remainderBytes));
                int temp = send(sockfd,sendline,strlen(sendline),0);
                if (temp < 0){
                    perror("client not sending properly");
                    exit(1);
                }
                bzero(sendline,1024);
                sprintf(sendline, "%d", (long) (((avgBytes + remainderBytes)*tempVal)  + (avgBytes * counter)));
                remainderBytes = 0;
                tempVal = 1;
            }
            //sends the message to the server 
            if( send(sockfd , sendline , strlen(sendline) , 0) < 0)
            {
                perror("Send failed\n");
                exit(1);
            }             

            //while loop that ends only when the server is done
            //sending its messages to the client            
            while( (recfd = recv(sockfd , recvline , MAXLINE , 0)) > 0)
            {

            //The file size and average bytes per connection and
            //leftover bytes are all set below
            if(setFileBytes == 0){
                printf("GOT INTO SETFILEBYTES TWO\n");
                setFileBytes = 1;
                fileSize = atoi(recvline);
                avgBytes = fileSize/minimum;
                remainderBytes = fileSize%minimum;
                break;
            }

            
            //variables created for allowing search of
            //the words "empty" and "exit" in recieved data
            int myLength = strlen(recvline);
            const char *last_five = &recvline[myLength-5];
            const char *last_four = &recvline[myLength-4];	    


                //if client enters exit, this allows the
                //client to close its socket and exit
                if (strncmp(last_four,"exit",4) == 0){
                /*
                  int buffLength = strlen(recvline) - 5;
                  printf("%.*s",buffLength,recvline + 0);
                */
                  close(sockfd);
                  setBreak = 1;
                  break;
                }

            /*
                //this message is for when the server is done
                //sending its messages. It allows the client to 
                //enter its next command.
                if(strncmp(last_five,"empty",5) == 0){

                    //allows client to print data up until "empty" 
                    //is also sent back by server
                    int buffLength = strlen(recvline) - 5;
                    printf("%.*s",buffLength,recvline + 0);
                    break;
                }

            */
              if(setSendBytes == 1){
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
            }

            bzero(recvline,MAXLINE); //the recieving buffer is reset/zeroed
        }//close second while loop
    minimum--;
    counter++;
    sockfd = createSocket();
  }//close first while loop
  fclose(fp); //fclose comes here?
  printf("COULD COME HER BUT NOT SUPPOSED TO\n");
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
    int sockfd = createSocket();

    //createServerHelper();
    //createServerHelper();
    //createServerHelper();

    //int sockfd = createSocket();
    //createServer(argv[1],argv[2]);
    //connectSocket(sockfd);
    readWriteSocket(sockfd,argv[3]);
    return 0;
}
