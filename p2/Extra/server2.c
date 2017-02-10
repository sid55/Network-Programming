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

#define MAXLINE2 4096 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int     listenfd, connfd,read_size; //the listen and accept file desciptors
struct sockaddr_in servaddr; //the server address
char    buff[MAXLINE2]; //the buffer which reads and sends lines
time_t ticks; //ticks variable 
int port; //server port number
FILE *in; //will be used with popen
extern FILE *popen(); //will be used with popen

FILE *fileRead;

/*
 * This method checks the number of arguments when running
 * the server. It makes sure the number of arguments does
 * not exceed 2. An error will be thrown otherwise.
 */
void numArgs(int argc){
    if (argc != 2){
        perror("usage: a.out <PORTnumber>\n");
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
        perror("socket error\n");
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
    sscanf(portnum,"%d",&port); //converts portnum to int
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port); //sets the port number here
   
    /* 
    char ipBuffer[80] = "128.32.16.1";
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0){
            perror("inet_pton error");
            exit(1);
    }
    */
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
        perror("bind error\n");
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
        perror("listen error\n");
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
            perror("accept error\n");
            exit(1);
        } 
        return connfd;
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

int y = 0; //used to signal second phase of reading in data

  while( (read_size = recv(connfd , buff , MAXLINE2 , 0)) > 0 )
    {
        //makes it possible to put the buffer
        //into a file. popen executes the command
        //that the client gives to it.
        if (!(in = popen(buff, "r"))) {
          perror("Not able to read stream");
          exit(1);
        } 

	//this int value will be used to ensure that
	//that fgets doesnt get called twice when only is
	//supposed to get called once
        int x = 0;

        
        //if user enters nothing or for other cases like
        //that, the if else send a message called 
        //"empty" back to the client 
        if(fgets(buff,sizeof(buff), in) == NULL){
            if(strncmp(buff,"exit",4) == 0){
              sprintf(buff,"exit");
              write(connfd,buff,strlen(buff));
              bzero(buff,MAXLINE2);
              x = 1;
            }else if(strncmp(buff,"empty",5)==0){
              sprintf(buff,"empty");
              write(connfd, buff, strlen(buff));
              bzero(buff,MAXLINE2);
              x = 1;
            }else if(y == 1){
              printf("got into second part of server, buff is: %s\n",buff);
              bzero(buff,MAXLINE2);
              sprintf(buff,"hello");
              write(connfd, buff, strlen(buff));
              x = 1;
            }else{

              fileRead = fopen(buff,"r");
              printf("SUCCESS OPENING FILE\n");
              if (fileRead == NULL){
                  perror("The file you are trying to read does not exist\n");
                  exit(1);
              }

              /*
               * Size of file being read set in global variable
               */
              fseek(fileRead,0L,SEEK_END);
              int fileSize = ftell(fileRead);
              rewind(fileRead);
           
              bzero(buff,MAXLINE2);
              sprintf(buff,"%d",(long)fileSize); 
              write(connfd, buff, strlen(buff));
              bzero(buff,MAXLINE2); 
              x = 1;
              y = 1; 
            }
           
        }else{
            printf("%s", buff);
            write(connfd , buff , strlen(buff));
            bzero(buff,MAXLINE2);
        }
        
        //this while loop continually reads from the "in"
        //and puts in the results of popen, line by line
        //into the buffer which gets sent back to the client
        while (fgets(buff, sizeof(buff), in) != NULL) {
            printf("%s", buff);
            write(connfd , buff , strlen(buff));
            bzero(buff,MAXLINE2);
        }
  
        //this if statement sends a final message to the client
        //letting it know its done sending messages and allows the
        //client to type a new command
        if((fgets(buff,sizeof(buff), in) == NULL) && (x==0)){
            bzero(buff,MAXLINE2);
            sprintf(buff,"empty");
            write(connfd, buff, strlen(buff));
            bzero(buff,MAXLINE2);
        }
        pclose(in);
        bzero(buff,MAXLINE2); //zeroes/resets the buffer
    }
 
    //this is for when the client is done sending
    //its messages to the server. The client disconnects.     
    if(read_size == 0)
    {
        close(connfd); //closes the file descriptor returned by accept
    }
    else if(read_size < 0)
    {
        perror("recv failed on server\n");
    }
}

/*
 * This method allows the connection to the server to happen
 * for the client with the accept system call. It then calls
 * the main meat of this program, readWriteServer which is 
 * a method that is briefly explained above. There is a while
 * loop to ensure that when the client exits, a new connection
 * can be established with the server.
 */
void acceptReadWriteServer(int listenfd){
  while(1){
    int connfd = acceptServer(listenfd);
    readWriteServer(connfd, listenfd); //contains code for receiving and sending data
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
  acceptReadWriteServer(listenfd);
  return 0;
}
