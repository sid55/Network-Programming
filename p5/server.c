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
#include <ctype.h>

#define MAXLINE2 4096 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int     listenfd, connfd,read_size; //the listen and accept file desciptors
struct sockaddr_in servaddr; //the server address
char    sendBuff[MAXLINE2], recvBuff[MAXLINE2]; //the buffer which reads and sends lines

/*
 * This method checks the number of arguments when running
 * the server. It makes sure the number of arguments does
 * not exceed 2. An error will be thrown otherwise.
 */
void numArgs(int argc){
    if (argc != 2){
        printf("usage: server <listen-port>\n");
        exit(1);
    }
}

/*
 * This method creates a socket on the server. This
 * is the socket that will be listened on and an error
 * will be thrown if there is a problem creating
 * this socket. If no error, the listenfd is returned.
 */
int createListenSocket(){
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0){
        printf("socket error\n");
        exit(1);
    }
    return listenfd;
}

/*
 * This method initializes the characteristics of the 
 * server and gives it the port number specified by the
 * user.  
 */
void createServer(const char *portnum){
    int port;
    sscanf(portnum,"%d",&port); //converts portnum to int
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port); //sets the port number here
}

/*
 * This method binds the port using the listen file 
 * descriptor. If there is an error with this process
 * an error will be thrown. An error is thrown if that
 * port is being used for other purposes.
 */
void bindServer(int listenfd){
    int bindID = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (bindID < 0){
        printf("bind error\n");
        exit(1);
    } 
}

/*
 * This method listend forever waiting for clients to connect
 * to this server. When a client is connected it is added
 * to the listen queue. An error is thrown otherwise.
 */
void listenServer(int listenfd){
    int listenID = listen(listenfd, LISTENQ); //the listen queue is LISTENQ
    if (listenID < 0){
        printf("listen error\n");
        exit(1);
    } 
}

/*
 * This method uses the accept system call which finds a diff
 * port on the server for the client to communicate with.
 * This is to allow other clients to communicate with the 
 * server's port while the original client can still communicate 
 * with the server. An error is thrown if a problem occurs.
 */
int acceptServer(int listenfd){
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd < 0){
            printf("accept error\n");
            exit(1);
        } 
        return connfd;
}

//Count number of words in a buffer
int countWords (const char* str)
{
  int count = 0;

  while (*str != '\0')
  {
    while (*str != '\0' && isblank(*str)) // remove all spaces between words
    {
      str++;
    }
    if(*str != '\0')
    {
      count++;
    }

    while (*str != '\0' && !isblank(*str)) // loop past the found word
    {
      str++;
    }
  }

  return count;
}


/*
 * This method is the main meat of this program. It has an
 * infinite while loop, that will continuously read input from
 * the client and execute those commands that were sent to it.
 * It does so by using the popen system call which basically
 * opens a file and inputs the execution into it which later
 * gets sent to a buffer. This buffer is then sent to the client
 * who prints the output onto the screen.
 */
void readWriteServer(int connfd, int listenfd){
    while(1){
        int read_size;
        int send_size;
        bzero(recvBuff, MAXLINE2);
        bzero(sendBuff, MAXLINE2);
        if ( (read_size = recv(connfd, recvBuff, MAXLINE2, 0)) > 0){
            printf("recieved content: \n%s", recvBuff); 
        }
       
        int wordCount = countWords(recvBuff);
        printf("word count is: %d\n", wordCount); 
        sprintf(sendBuff, "server content");

        if ( (send_size = send(connfd, sendBuff, MAXLINE2, 0)) > 0){
            printf("sent content\n\n");
        }
    }
}

/*
 * This is the class's main which gets executed when this 
 * program is run.
 */
int
main(int argc, char **argv)
{
  numArgs(argc);
  int listenfd = createListenSocket(); //the file desriptor for the socket on server
  createServer(argv[1]);
  bindServer(listenfd);
  listenServer(listenfd);
  int connfd = acceptServer(listenfd);
  readWriteServer(connfd, listenfd);
  return 0;
}
