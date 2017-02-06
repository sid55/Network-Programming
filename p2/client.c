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
int counter = 0; //set after first connection, only used to send leftover bytes -------------maybe unneeded-----------
int remainderBytes = 0; //set as leftover bytes after dividing file size by minimum
                        //value is added to only first connection
int avgBytes = 0; //set as average number of bytes to send per connection

/*
 * This method checks to make sure that the number of 
 * arguments is not invalid and above 3. It will
 * throw an error otherwise.
 */
void numArgs(int argc){
    if (argc != 3){
        perror("usage: a.out <server-info.txt> <num-connections>\n");
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
    }
}

/*
 * Get the file's size that you need to read and also find the minimum
 * between the number of connections and the number of servers
 */
void getFileSizeAndMin(const char *fileToRead,const char *numConnections){
    int tempNumConnections = -1;
    int tempNumLines = 0;
    int ch = 0;

    fileRead = fopen(fileToRead,"r");
    if (fileRead == NULL){
        perror("The file you are trying to read does not exist\n");
        exit(1);
    }

    /*
     * Size of file being read set in global variable
     */
    fseek(fileRead,0L,SEEK_END);
    fileSize = ftell(fileRead);
    rewind(fileRead);

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

    avgBytes = fileSize/minimum;
    remainderBytes = fileSize%minimum;
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
void readWriteSocket(int sockfd){
  int recfd; //the file descriptor for recieving messages

  while(minimum > 0){
    createServerHelper();
    connectSocket(sockfd); 
      //infinite while loop that exits either during an error
      //or when the client sends an "exit" message
      while(1){
            bzero(sendline,1024); //zeroing out the buffer
            printf("> ");
            fgets(sendline, 1024, stdin); //gets user input
            
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

            //variables created for allowing search of
            //the words "empty" and "exit" in recieved data
            int myLength = strlen(recvline);
            const char *last_five = &recvline[myLength-5];
            const char *last_four = &recvline[myLength-4];	    


                //if client enters exit, this allows the
                //client to close its socket and exit
                if (strncmp(last_four,"exit",4) == 0){
                  close(sockfd);
                  exit(0);
                }


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


              fputs(recvline,stdout); //prints data received onto screen
              bzero(recvline,MAXLINE); //zero out buffer


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
  }//close first while loop
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
    getFileSizeAndMin("server.c",argv[2]);
    printf("end of main, size of file read is: %d \n",fileSize);
    int sockfd = createSocket();

    //createServerHelper();
    //createServerHelper();
    //createServerHelper();

    
    //int sockfd = createSocket();
    //createServer(argv[1],argv[2]);
    //connectSocket(sockfd);
    readWriteSocket(sockfd);
    return 0;
}