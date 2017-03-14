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
FILE *logReader;
pthread_mutex_t lock;

typedef struct{
    //Changed from MAXLINE + 1 to MAXLINE and 1024 to MAXLINE
    char recvline[MAXLINE]; //the recieve buffer 
    char sendline[MAXLINE]; //the send buffer
    int thread_id; //the thread id
    struct sockaddr_in servaddr; //address of server
    int listenfd, connfd; //file descriptors
    FILE *reader; //file reader to read forbidden list of websites
    char forbiddenList[MAXLINE]; 
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
 * not exceed 2. An error will be thrown otherwise. It also
 * created the thread array and the thread_id  array. The last
 * thing it does is create the access.log to store all the 
 * requests into. 
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

    //set up access log to put entires into
    if (access("access.log",F_OK)!=-1){
	remove("access.log");
    }
}

/*
 * This method sets up some of the parts of the thread array. It loops
 * through the elements setting up some elements
 */
void setUpThreadArray(const char *forbiddenList){

    /*
    if (access(forbiddenList,F_OK)==1){
	printf("The forbidden list file does not exist\n");
	exit(1);
    }
    */
    FILE *tempo = fopen(forbiddenList, "r");
    if (tempo == NULL){
	printf("The forbidden list file does not exist\n");
	exit(1);
    }else{
	fclose(tempo);
    }	 

    int i = 0;
    while (i < maxThreads){
        thread_pnt[i].thread_id = i;	
	sprintf(thread_pnt[i].forbiddenList, "%s", forbiddenList);
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

    //convert in_addr_t to char* with ip addr of proxy
    //in_addr_t x = servaddr.sin_addr.s_addr;
    //char *z;
    //z = inet_ntoa(*(struct in_addr *)&x);
    char z[MAXLINE];
    bzero(z, MAXLINE);
    uint32_t tempLen = sizeof(servaddr);
    int myProxyNum = getsockname(listenfd, (struct sockaddr *)&servaddr, &tempLen);
    sprintf(z,inet_ntoa(servaddr.sin_addr));

    //have each thread struct hold the proxy address
    int i = 0;
    while (i <maxThreads){
        sprintf(thread_pnt[i].proxyAddr, "%s", z); 
        i++;
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
      //create browseraddr struct here
      struct sockaddr_in browseraddr;
      uint32_t addrlen = sizeof(browseraddr); //length of addresses
      bzero(&browseraddr, sizeof(struct sockaddr_in));

      //accept connection here
      int connfd;
      connfd = accept(threadInfo->listenfd, (struct sockaddr *) &browseraddr, &addrlen);
      if (connfd < 0){
          perror("accept error\n");
          exit(1);
      }

      char myBrowserIpaddr[MAXLINE];
      bzero(myBrowserIpaddr, MAXLINE);
      int myIpNum = getsockname(connfd, (struct sockaddr *)&browseraddr, &addrlen);
      sprintf(myBrowserIpaddr,inet_ntoa(browseraddr.sin_addr));

      threadInfo->reader = fopen(threadInfo->forbiddenList,"r");

      char logBuf[MAXLINE];
      bzero(logBuf,MAXLINE); 		
      struct timeval tv;

      //initialize vals of thread
      bzero(threadInfo->recvline, MAXLINE);
      bzero(threadInfo->sendline, MAXLINE);
      threadInfo->connfd = connfd;

      
	
      int read_size = 0;
      int problemSet = 0; 
      if ( (read_size = recv(threadInfo->connfd, threadInfo->recvline, MAXLINE, 0)) > 0){
	char extraBuff[MAXLINE];
	bzero(extraBuff, MAXLINE);
	sprintf(extraBuff, "%s", threadInfo->recvline);
	char request_first_line[MAXLINE];
	bzero(request_first_line, MAXLINE);
	char * pch4;
	pch4 = strtok(extraBuff,"\n");
	strcpy(request_first_line,pch4);

	printf("Send buffer: \n%s\n\n",threadInfo->recvline);

	if (read_size > 9000){
	    printf("Invalid address\n");
	    //SEND: Need to send message back to browser of: too long url
	    char errorMsg[MAXLINE];
		bzero(errorMsg, MAXLINE);
		sprintf(errorMsg,
		"HTTP/1.0 413 Request Entity Too Large\r\n\
		Content-Type: text/html\r\n\
		Connection: close\r\n\n\
		<html>\r\n\
			<head>\r\n\
				<title>413 - Request Entity Too Large</title>\r\n\
			</head>\r\n\
			<body>\r\n\
				<h1>413 - Request Entity Too Large</h1>\r\n\
			</body>\r\n\
		</html>");

		if(send(connfd , errorMsg , MAXLINE , 0) < 0)
		{
			printf("Send failed when sending to browser\n");
			exit(1);     
		}
		pthread_mutex_lock(&lock);

		//get current time
		char timeBuf[MAXLINE];
		bzero(timeBuf, MAXLINE);
		time_t rawtime;
		struct tm *timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(timeBuf, MAXLINE, "%xT%XZ", timeinfo);

		request_first_line[(int)strlen(request_first_line) - 1] = '\0';
		FILE *logReader = fopen("access.log", "a");
		sprintf(logBuf, "%s %s %s 413 %d", timeBuf, myBrowserIpaddr,request_first_line, (int)strlen(errorMsg));
		fputs(logBuf, logReader);
		fputs("\n", logReader);
		fclose(logReader);
		pthread_mutex_unlock(&lock);

	}else{

	    char command[MAXLINE]; char website[MAXLINE]; char http[MAXLINE]; char sendBuff[MAXLINE]; char sendBuff2[MAXLINE]; 
	    bzero(command,MAXLINE);
	    bzero(website,MAXLINE);
	    bzero(http, MAXLINE);
	    bzero(sendBuff, MAXLINE);
	    bzero(sendBuff2, MAXLINE);

	    char myForwardedHeader[MAXLINE];
	    bzero(myForwardedHeader, MAXLINE);
	    sprintf(myForwardedHeader, "Forwarded: for=%s;proto=http;by=%s", myBrowserIpaddr, threadInfo->proxyAddr);

	    int location = (int)strlen(threadInfo->recvline) - 1;
	    threadInfo->recvline[location] = '\0';
	    sprintf(threadInfo->recvline + strlen(threadInfo->recvline), "%s\r\n\n", myForwardedHeader);


	    sprintf(sendBuff, "%s", threadInfo->recvline);
	    sprintf(sendBuff2, "%s", threadInfo->recvline);

	    char * pch;
	    pch = strtok(threadInfo->recvline," ");
	    strcpy(command,pch);

	    if (strncmp(command, "GET", 3) == 0 || strncmp(command, "HEAD", 4) == 0){

  
		//get rid of http(s) and add in www and get rid of last '/' 
		char web[MAXLINE]; char webTemp[MAXLINE]; //char httpsFront[MAXLINE];
		char webForbidden[MAXLINE];
		bzero(webForbidden, MAXLINE);
		bzero(web, MAXLINE);
		bzero(webTemp, MAXLINE);

		
		char *pch2; 
		pch2 = strtok(sendBuff2, "\n");
		pch2 = strtok(NULL, " ");
		pch2 = strtok(NULL, "\n");
		strcpy(webTemp, pch2);
		int len = (int)strlen(webTemp) - 1;
		memcpy(web, &webTemp[0], len);
		sprintf(webForbidden, "www.%s", webTemp);

		//Debugging Info
		//printf("web created is: %s\n", web);
		//printf("webtemp is: %s\n", webTemp);
		//printf("webForbidden is: %s\n", webForbidden);
		//printf("the strlen is: %d\n", (int)strlen(webForbidden));
		
		int useless = 0;
		if (useless == 1){
			printf("potential error checking for https\n");
		}else{ 
			//make sure website is not part of forbidden list
			int boolean = 0; //is true
			char line[MAXLINE];
			bzero(line, MAXLINE);
			while(fgets(line, MAXLINE, (FILE *)threadInfo->reader)){
			    if (strncmp(line, webForbidden, (int)strlen(webForbidden) - 1) == 0){
				boolean = 1;
			    } 
			}
			if (boolean == 1){
			    //SEND: send message back to browser that website cannot be used because it is in forbidden list  
			    printf("Gets into right place of forbidden\n"); 
			    char errorMsg[MAXLINE];
			    bzero(errorMsg, MAXLINE);
				sprintf(errorMsg,
				"HTTP/1.0 403 Forbidden\r\n\
				Content-Type: text/html\r\n\
				Connection: close\r\n\n\
				<html>\r\n\
					<head>\r\n\
						<title>403 - Forbidden</title>\r\n\
					</head>\r\n\
					<body>\r\n\
						<h1>403 - Forbidden</h1>\r\n\
					</body>\r\n\
				</html>");

				if(send(connfd , errorMsg , MAXLINE , 0) < 0)
				{
					printf("Send failed when sending to browser\n");
					exit(1);     
				}
				pthread_mutex_lock(&lock);
				//get current time
				char timeBuf[MAXLINE];
				bzero(timeBuf, MAXLINE);
				time_t rawtime;
				struct tm *timeinfo;
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(timeBuf, MAXLINE, "%xT%XZ", timeinfo);

				request_first_line[(int)strlen(request_first_line) - 1] = '\0';
				FILE *logReader = fopen("access.log", "a");
				sprintf(logBuf, "%s %s %s 403 %d", timeBuf, myBrowserIpaddr,request_first_line, (int)strlen(errorMsg));
				fputs(logBuf, logReader);
				fputs("\n", logReader);
				fclose(logReader);
				pthread_mutex_unlock(&lock);

		 
			}else{
	
	
			    int sockfd, n;
			    struct addrinfo hints, *res;
			    bzero(&hints, sizeof(struct addrinfo));
			    hints.ai_family = AF_UNSPEC;
			    hints.ai_socktype = SOCK_STREAM;	     
			   

			    if((n = getaddrinfo(web,"80",&hints, &res)) != 0){
				char errorMsg[MAXLINE];
				bzero(errorMsg, MAXLINE);
				sprintf(errorMsg,
				"HTTP/1.0 404 Not Found\r\n\
				Content-Type: text/html\r\n\
				Connection: close\r\n\n\
				<html>\r\n\
					<head>\r\n\
						<title>404 - Not Found</title>\r\n\
					</head>\r\n\
					<body>\r\n\
						<h1>404 - Not Found</h1>\r\n\
					</body>\r\n\
				</html>");
		
				if(send(connfd , errorMsg , MAXLINE , 0) < 0)
				{
					printf("Send failed when sending to browser\n");
					exit(1);     
				}
				pthread_mutex_lock(&lock);
				//get current time
				char timeBuf[MAXLINE];
				bzero(timeBuf, MAXLINE);
				time_t rawtime;
				struct tm *timeinfo;
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(timeBuf, MAXLINE, "%xT%XZ", timeinfo);

				request_first_line[(int)strlen(request_first_line) - 1] = '\0';
				FILE *logReader = fopen("access.log", "a");
				sprintf(logBuf, "%s %s %s 404 %d", timeBuf, myBrowserIpaddr,request_first_line, (int)strlen(errorMsg));
				fputs(logBuf, logReader);
				fputs("\n", logReader);
				fclose(logReader);
				pthread_mutex_unlock(&lock);

			    }else{

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
					char errorMsg[MAXLINE];
					bzero(errorMsg, MAXLINE);
					sprintf(errorMsg,
					"HTTP/1.0 502 Bad Gateway\r\n\
					Content-Type: text/html\r\n\
					Connection: close\r\n\n\
					<html>\r\n\
						<head>\r\n\
							<title>502 - Bad Gateway</title>\r\n\
						</head>\r\n\
						<body>\r\n\
							<h1>502 - Bad Gateway</h1>\r\n\
						</body>\r\n\
					</html>");
			
					if(send(connfd , errorMsg , MAXLINE , 0) < 0)
					{
						printf("Send failed when sending to browser\n");
						exit(1);     
					}
					pthread_mutex_lock(&lock);
					//get current time
					char timeBuf[MAXLINE];
					bzero(timeBuf, MAXLINE);
					time_t rawtime;
					struct tm *timeinfo;
					time(&rawtime);
					timeinfo = localtime(&rawtime);
					strftime(timeBuf, MAXLINE, "%xT%XZ", timeinfo);

					request_first_line[(int)strlen(request_first_line) - 1] = '\0';
					FILE *logReader = fopen("access.log", "a");
					sprintf(logBuf, "%s %s %s 502 %d", timeBuf, myBrowserIpaddr,request_first_line, (int)strlen(errorMsg));
					fputs(logBuf, logReader);
					fputs("\n", logReader);
					fclose(logReader);
					pthread_mutex_unlock(&lock);

					close(sockfd);
				    }else{


					    if(send(sockfd , sendBuff , MAXLINE , 0) < 0)
					    {
						printf("Send failed\n");
						exit(1);     
					    } 

					    int recfd2;
					    char recvBuff[MAXLINE];
					    bzero(recvBuff, MAXLINE);
					    //set timeout value for read from browser
					    tv.tv_sec = 8;
					    tv.tv_usec = 0;
					    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)); 
	

					    if( (recfd2 = recv(sockfd , recvBuff , MAXLINE , 0)) > 0)
					    {
						   
							    printf("recieved this from server: \n%s\n\n", recvBuff); 
							    if(send(connfd , recvBuff , MAXLINE	 , 0) < 0)
							    {
								printf("Send failed when sending to browser\n");
								exit(1);     
							    }else{
								pthread_mutex_lock(&lock);

								//get current time
								char timeBuf[MAXLINE];
								bzero(timeBuf, MAXLINE);
								time_t rawtime;
								struct tm *timeinfo;
								time(&rawtime);
								timeinfo = localtime(&rawtime);
								strftime(timeBuf, MAXLINE, "%xT%XZ", timeinfo);

								//get statusNum, recfd2 
								char copy[MAXLINE]; char status[MAXLINE];
								bzero(copy, MAXLINE);
								bzero(status, MAXLINE);
							        sprintf(copy, "%s", recvBuff);
							        char *pch7; 
							        pch7 = strtok(copy, " ");
							        pch7 = strtok(NULL, " ");
							        strcpy(status, pch7);
							        int statusNum = atoi(status);


								//Attempt at getting the content length from the recieved buffer
								/*
								int myContentLength = 0;
							  	char contentLength[MAXLINE]; char nameGoal[MAXLINE];
								bzero(nameGoal, MAXLINE);
								bzero(contentLength, MAXLINE);
								if (strncmp(command, "GET", 3) == 0){
									pch7 = strtok(NULL, "\n");
									pch7 = strtok(NULL, " ");
									strcpy(nameGoal, pch7);
									while(strncmp(nameGoal, "Content-Length:", 15) != 0 ){
										bzero(nameGoal, MAXLINE);
										pch7 = strtok(NULL, "\n");
										pch7 = strtok(NULL, " ");
										strcpy(nameGoal, pch7);
										printf("name goal is: %s length: %d\n character:%c", nameGoal, (int)strlen(nameGoal), nameGoal[0]);
									}
									pch7 = strtok(NULL, " ");
									sprintf(contentLength, pch7);
									printf("The content-lenght is: %s\n", contentLength);
									myContentLength = atoi(contentLength);	
								}else{
									myContentLength = 0;
								}
								*/
								request_first_line[(int)strlen(request_first_line) - 1] = '\0';
								FILE *logReader = fopen("access.log", "a");
								sprintf(logBuf, "%s %s %s %d %d", timeBuf, myBrowserIpaddr,request_first_line, statusNum,recfd2);
								fputs(logBuf, logReader);
								fputs("\n", logReader);
								fclose(logReader);
								pthread_mutex_unlock(&lock);
							    } 
						    bzero(recvBuff, MAXLINE);
					    }
					    if (recfd2 < 0){
						//Send an error back to server of too long wait
						char errorMsg[MAXLINE];
						bzero(errorMsg, MAXLINE);
						sprintf(errorMsg,
						"HTTP/1.0 504 Gateway Timeout\r\n\
						Content-Type: text/html\r\n\
						Connection: close\r\n\n\
						<html>\r\n\
							<head>\r\n\
								<title>504 - Gateway Timeout</title>\r\n\
							</head>\r\n\
							<body>\r\n\
								<h1>504 - Gateway Timeout</h1>\r\n\
							</body>\r\n\
						</html>");
				
						if(send(connfd , errorMsg , MAXLINE , 0) < 0)
						{
							printf("Send failed when sending to browser\n");
							exit(1);     
						}
						pthread_mutex_lock(&lock);
						//get current time
						char timeBuf[MAXLINE];
						bzero(timeBuf, MAXLINE);
						time_t rawtime;
						struct tm *timeinfo;
						time(&rawtime);
						timeinfo = localtime(&rawtime);
						strftime(timeBuf, MAXLINE, "%xT%XZ", timeinfo);

						request_first_line[(int)strlen(request_first_line) - 1] = '\0';
						FILE *logReader = fopen("access.log", "a");
						sprintf(logBuf, "%s %s %s 504 %d", timeBuf, myBrowserIpaddr,request_first_line, (int)strlen(errorMsg));
						fputs(logBuf, logReader);
						fputs("\n", logReader);
						fclose(logReader);
						pthread_mutex_unlock(&lock);
					    }
					    printf("Done with recieving data with recvfd: %d\n", recfd2);
					    close(sockfd);
				    }
			    }  
			}
		}

	    }else if(strncmp(command, "HEAD", 4) == 0){
		printf("Came into HEAD part\n");
	    }else{
		//SEND: send message back to client of can't do intruction
			char errorMsg[MAXLINE];
			bzero(errorMsg, MAXLINE);
			sprintf(errorMsg,
			"HTTP/1.0 501 Not Implemented\r\n\
			Content-Type: text/html\r\n\
			Connection: close\r\n\n\
			<html>\r\n\
				<head>\r\n\
					<title>501 - Not Implemented</title>\r\n\
				</head>\r\n\
				<body>\r\n\
					<h1>501 - Not Implemented</h1>\r\n\
				</body>\r\n\
			</html>");
	
			if(send(connfd , errorMsg , MAXLINE , 0) < 0)
			{
				printf("Send failed when sending to browser\n");
				exit(1);     
			}

			pthread_mutex_lock(&lock);
			//get current time
			char timeBuf[MAXLINE];
			bzero(timeBuf, MAXLINE);
			time_t rawtime;
			struct tm *timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(timeBuf, MAXLINE, "%xT%XZ", timeinfo);

			request_first_line[(int)strlen(request_first_line) - 1] = '\0';
			FILE *logReader = fopen("access.log", "a");
			sprintf(logBuf, "%s %s %s 501 %d", timeBuf, myBrowserIpaddr,request_first_line, (int)strlen(errorMsg));
			fputs(logBuf, logReader);
			fputs("\n", logReader);
			fclose(logReader);
			pthread_mutex_unlock(&lock);

	    }
 
	    bzero(threadInfo->recvline, MAXLINE); 
	}   
      }
      
      if (read_size == 0){
	printf ("Done receiving data \n");
      }

      fclose(threadInfo->reader);
      close(connfd);
   }//close inside infinite while loop //can delete this while loop??
   return NULL;
}//end method

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
  if(pthread_mutex_init(&lock, NULL) != 0){
	printf("Mutex cannot be initialized correctly\n");
	exit(1);
  }
  acceptServer(listenfd);
  return 0;
}   
