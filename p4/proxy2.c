#include <time.h>
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

#define MAXLINE 4096 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int maxThreads = 512;
int threadIndex = 0;
struct sockaddr_in servaddr;


typedef struct{
    char recvline[MAXLINE + 1]; //the recieve buffer 
    char sendline[1024]; //the send buffer
    int thread_id; //the thread id
    struct sockaddr_in servaddr;
    int listenfd, connfd;
}Thread;

//Thread pointer created -> pointer to an array of structs
Thread *thread_pnt;

//Pointer to array of pthread_t variables
pthread_t *thread_pnt2;

//The mutex lock variable
pthread_mutex_t lock;


/*
 * This method checks the number of arguments when running
 * the server. It makes sure the number of arguments does
 * not exceed 2. An error will be thrown otherwise.
 */
void numArgs(int argc){
    if (argc != 3){
        perror("usage: a.out <listen-port> <forbidden-sites-list>\n");
        exit(1);
    }

    //Set up two pointer arrays, one for pthread_t variables and
    //other for the Thread array of structs
    thread_pnt = (Thread*)malloc(maxThreads*sizeof(Thread));
    thread_pnt2 = (pthread_t*)malloc(maxThreads*sizeof(pthread_t));

    printf("Checked arguments\n");
}


/*
 * This method creates a socket on the server. This
 * is the socket that will be listened on and an error
 * will be thrown if there is a problem creating
 * this socket. If no error, the listenfd is returned.
 */
int createListenSocket(){
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0){
        perror("socket error\n");
        exit(1);
    }
    return listenfd;
    
    printf("Created listening socket\n");
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

    printf("Set up server to recieve data from browser\n");
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
    
    printf("Binded the server\n"); 
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

    printf("Server able to listen\n"); 
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
void *readWriteServer(void *threadInfoTemp){
  Thread *threadInfo = (Thread *)threadInfoTemp;

  int connfd;
  connfd = accept(threadInfo->listenfd, (struct sockaddr *) NULL, NULL);
  if (connfd < 0){
      perror("accept error\n");
      exit(1);
  }

  bzero(threadInfo->recvline, MAXLINE);
  bzero(threadInfo->sendline, MAXLINE);
  threadInfo->connfd = connfd;

  struct sockaddr_in browseraddr;
  
  while(1){

      int read_size = 0;

      while( (read_size = recv(threadInfo->connfd , threadInfo->recvline , MAXLINE , 0)) > 0 ){
        printf ("Recieved data: %s \n", threadInfo->recvline);
        bzero(threadInfo->recvline, MAXLINE);    
      }

      if (read_size == 0){
        printf ("Done receiving data \n");
        break;
      }

  }
  return NULL;
}

/*
 * This method uses the accept system call which finds a diff
 * port on the server for the client to communicate with.
 * This is to allow other clients to communicate with the 
 * server's port while the original client can still communicate 
 * with the server. An error is thrown if a problem occurs.
 */
int acceptServer(int listenfd){
    while(1){ 

	int i = 0;
	while (i < maxThreads){ 
	     thread_pnt[i].listenfd = listenfd; 
	     if(pthread_create(&thread_pnt2[i],NULL,readWriteServer,&thread_pnt[i])){
	         printf("problem creating a thread\n");
	         exit(1);
	     }
	     i++; 
	} 

	i = 0;
	while (i < maxThreads){
             if(pthread_join(thread_pnt2[i],NULL)){
                 printf("problem joining a thread\n");
                 exit(1);
             } 
	     i++;
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
  acceptServer(listenfd);
  return 0;
}   
