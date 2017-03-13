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

#define MAXLINE 9216 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

int maxThreads = 256;
int threadIndex = 0;
struct sockaddr_in servaddr;


typedef struct{
    //Changed from MAXLINE + 1 to MAXLINE and 1024 to MAXLINE
    char recvline[MAXLINE]; //the recieve buffer 
    char sendline[MAXLINE]; //the send buffer
    int thread_id; //the thread id
    struct sockaddr_in servaddr; //address of server
    int listenfd, connfd; //file descriptors
    FILE *reader; //file reader to read forbidden list of websites 
    char proxyAddr[MAXLINE];
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

}

/*
 * This method sets up some of the parts of the thread array. It loops
 * through the elements setting up some elements
 */
void setUpThreadArray(const char *forbiddenList){
    int i = 0;
    while (i < maxThreads){
        thread_pnt[i].thread_id = i;
        thread_pnt[i].reader = fopen(forbiddenList,"r");
        if (thread_pnt[i].reader == NULL){
            printf("The forbidden list is nonexistant\n");
            exit(1);
        } 
        i++;
    }
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

    //convert in_addr_t to char* with ip addr of proxy
    in_addr_t x = servaddr.sin_addr.s_addr;
    char *z;
    z = inet_ntoa(*(struct in_addr *)&x);

    //have each thread struct hold the proxy address
    int i = 0;
    while (i <maxThreads){
        sprintf(thread_pnt[i].proxyAddr, "%s", z); 
        i++;
    }
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
  while(1){
      //accept connection here
      printf("came to accept for thread number: %d\n", threadInfo->thread_id);
      int connfd;
      connfd = accept(threadInfo->listenfd, (struct sockaddr *) NULL, NULL);
      if (connfd < 0){
          perror("accept error\n");
          exit(1);
      }
      printf("Accpeted thread number: %d\n", threadInfo->thread_id);	

      //initialize vals of thread
      bzero(threadInfo->recvline, MAXLINE);
      bzero(threadInfo->sendline, MAXLINE);
      threadInfo->connfd = connfd;

      //create browseraddr struct here
      struct sockaddr_in browseraddr;
      uint32_t addrlen = sizeof(browseraddr); //length of addresses
      struct timeval tv;

	
      int read_size = 0;
      
      while(1){ 

	      //set timeout value for read from browser
	      //tv.tv_sec = 20;
	      //tv.tv_usec = 0;
	      //setsockopt(threadInfo->connfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)); 
	      while ( (read_size = recv(threadInfo->connfd, threadInfo->recvline, MAXLINE, 0)) > 0){
	      //while( (read_size = recvfrom(threadInfo->connfd, threadInfo->recvline, MAXLINE, 0, (struct sockaddr *)&browseraddr, &addrlen)) > 0 ){
		if (read_size > 9000){
		    printf("Invalid address\n");
		    //SEND: Need to send message back to browser of: too long url
		}else{

		    char command[MAXLINE]; char website[MAXLINE]; char http[MAXLINE]; char sendBuff[MAXLINE]; char sendBuff2[MAXLINE]; 
		    bzero(command,MAXLINE);
		    bzero(website,MAXLINE);
		    bzero(http, MAXLINE);
		    bzero(sendBuff, MAXLINE);
		    bzero(sendBuff2, MAXLINE);
		    sprintf(sendBuff, "%s", threadInfo->recvline);
		    sprintf(sendBuff2, "%s", threadInfo->recvline);

		    char * pch;
		    pch = strtok(threadInfo->recvline," ");
		    strcpy(command,pch);
		
		    if (strncmp(command, "GET", 3) == 0){
			printf("Came into GET part\n");

			//NEEDED http?: parse website and http type of 1.1 or 1.0
			//pch = strtok(NULL, " ");
			//strcpy(website,pch);
			//pch = strtok(NULL, " ");
			//strcpy(http,pch);

			//printf("website is: %s\n", website);
			//printf("http type is: %s\n", http); 

	  
			//get rid of http(s) and add in www and get rid of last '/' 
			char web[MAXLINE]; char webTemp[MAXLINE]; char httpFront[MAXLINE]; char httpsFront[MAXLINE];
			bzero(web, MAXLINE);
			bzero(webTemp, MAXLINE);

			
			char *pch2; 
			pch2 = strtok(sendBuff2, "\n");
			pch2 = strtok(NULL, " ");
			pch2 = strtok(NULL, "\n");
			strcpy(webTemp, pch2);
			int len = (int)strlen(webTemp) - 1;
			memcpy(web, &webTemp[0], len);

			//printf("web created is: %s\n", web);
			//printf("webtemp is: %s\n", webTemp);
			//printf("the strlen is: %d\n", (int)strlen(webTemp));
			
			/*
			bzero(httpFront, MAXLINE);
			bzero(httpsFront, MAXLINE);
			memcpy(httpFront, &website[0], 7);
			memcpy(httpsFront, &website[0], 8);
			if (strncmp(httpsFront, "https://", 8) == 0){
			    size_t lenWebsite = strlen(website);
			    int lastChar = (int)lenWebsite - 1;
			    int lengthToRead = lastChar - 8;
			    char substring[MAXLINE];
			    bzero(substring, MAXLINE);
			    memcpy(substring, &website[8], lengthToRead);
			    sprintf(web, "%s", substring);
			    //sprintf(web + strlen(web),"%s",substring);  
			    printf("The website created: %s\n", web);  
			}else if (strncmp(httpFront, "http://", 7) == 0){
			    size_t lenWebsite = strlen(website);
			    int lastChar = (int)lenWebsite - 1;
			    int lengthToRead = lastChar - 7;
			    char substring[MAXLINE];
			    bzero(substring, MAXLINE);
			    memcpy(substring, &website[7], lengthToRead);
			    sprintf(web, "%s", substring);
			    //sprintf(web + strlen(web),"%s",substring);  
			    printf("The website created: %s\n", web);  
			}
			*/

			//make sure website is not part of forbidden list
			int boolean = 0; //is true
			char line[MAXLINE];
			bzero(line, MAXLINE);
			while(fgets(line, MAXLINE, (FILE *)threadInfo->reader)){
			    if (strncmp(line, web, (int)strlen(web)) == 0){
				boolean = 1;
			    } 
			}
			if (boolean == 1){
			    //SEND: send message back to browser that website cannot be used because it is in forbidden list    
			}else{

			    int sockfd, n;
			    struct addrinfo hints, *res;
			    bzero(&hints, sizeof(struct addrinfo));
			    hints.ai_family = AF_UNSPEC;
			    hints.ai_socktype = SOCK_STREAM;	     
			   

			    if((n = getaddrinfo(web,"80",&hints, &res)) != 0){
				printf("Did no get address info\n");
				exit(1);
			    }
				   
	 
			    do{
				sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
				if (sockfd < 0)
					continue;

				if(connect (sockfd, res->ai_addr, res->ai_addrlen) == 0)
					break;
		
				close(sockfd);
			    }while((res = res->ai_next) != NULL);

			    if (res == NULL){
				printf("tcp connect error\n");
				exit(1);
			    }

			    if(send(sockfd , sendBuff , MAXLINE , 0) < 0)
			    {
				printf("Send failed\n");
				exit(1);     
			    } 


			    int recfd2;
			    char recvBuff[MAXLINE];
			    bzero(recvBuff, MAXLINE);
			    //set timeout value for read from browser
			    //tv.tv_sec = 20;
			    //tv.tv_usec = 0;
			    //setsockopt(threadInfo->connfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)); 
			    if( (recfd2 = recv(sockfd , recvBuff , MAXLINE , 0)) > 0)
			    {
				    printf("Recieved data: %s\n", recvBuff);
				    if(send(connfd , recvBuff , MAXLINE	 , 0) < 0)
				    {
					printf("Send failed\n");
					exit(1);     
				    } 
				    bzero(recvBuff, MAXLINE); 
			    }
			    printf("Done with recieving data with recvfd: %d\n", recfd2);
			    close(sockfd);  
			}

		    }else if(strncmp(command, "HEAD", 4) == 0){
			printf("Came into HEAD part\n");
		    }else{
			//SEND: send message back to client of can't do intruction
			printf("Not a valid instruction/command: %s\n",command);
		    }

	 
		    bzero(threadInfo->recvline, MAXLINE); 
		}   
	      }
	      }//close inside infinite while loop //can delete this while loop??

	      if (read_size == 0){
		printf ("Done receiving data \n");
		//Maybe this not needed?
	      }

	      if (read_size < 0){
		//SEND: Need to send message back to browser of: waited too long for browser message
		//					                       to reach proxy instead of breaking
		printf ("Thread has timed out on the socket recvFrom\n");
	      }

	      //Now need to connect to different server and send data and wait for response

	    close(connfd);
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
  setUpThreadArray(argv[2]);
  int listenfd = createListenSocket(); //the file desriptor for the socket on server
  createServer(argv[1]);
  bindServer(listenfd);
  listenServer(listenfd);
  acceptServer(listenfd);
  return 0;
}   
