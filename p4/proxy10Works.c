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
#include <errno.h>

#define MAXLINE 9216 //size of bytes for the buffer
#define LISTENQ 1024 //size of the listening queue of clients

struct sockaddr_in servaddr;
pid_t pid;

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
void readWriteServer(int listenfd){

  while(1){

      //accept connection here
      int connfd;
      connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
      if (connfd < 0){
          perror("accept error\n");
          exit(1);
      }

      //pid = fork(); 
      //if (pid == 0){ 
	      //printf("in child process\n");
	      //initialize buffers
	      char recvline[MAXLINE]; char sendline[MAXLINE];
	      bzero(recvline, MAXLINE);
	      bzero(sendline, MAXLINE);
		
	      int read_size = 0;
	      
	      while ( (read_size = recv(connfd, recvline, MAXLINE, 0)) > 0){
		if (read_size > 9000){
		    printf("Invalid address\n");
		    //SEND: Need to send message back to browser of: too long url
		}else{

		    char command[MAXLINE]; char sendBuff[MAXLINE]; char sendBuff2[MAXLINE]; 
		    bzero(command,MAXLINE);
		    bzero(sendBuff, MAXLINE); //used to send over to web server
		    bzero(sendBuff2, MAXLINE); //used for tokenizing
		    sprintf(sendBuff, "%s", recvline);
		    sprintf(sendBuff2, "%s", recvline);

		    char * pch;
		    pch = strtok(recvline," ");
		    strcpy(command,pch);
		
		    if (strncmp(command, "GET", 3) == 0){
			printf("\nCame into GET part\n\n");

			char web[MAXLINE]; char webTemp[MAXLINE]; 
			bzero(web, MAXLINE);
			bzero(webTemp, MAXLINE);
			
			char *pch2; 
			pch2 = strtok(sendBuff2, "\n");
			pch2 = strtok(NULL, " ");
			pch2 = strtok(NULL, "\n");
			strcpy(webTemp, pch2);
			int len = (int)strlen(webTemp) - 1;
			memcpy(web, &webTemp[0], len);

			printf("website is: %s length: %d\n", web, (int)strlen(web));
			
			/*
			//make sure website is not part of forbidden list
			int boolean = 0; //is true
			char line[MAXLINE];
			bzero(line, MAXLINE);
			while(fgets(line, MAXLINE, (FILE *)threadInfo->reader)){
			    if (strncmp(line, web, (int)strlen(web)) == 0){
				boolean = 1;
			    } 
			}
			*/

			int boolean = 0;
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

			    printf("res addr is: %u\n", res->ai_addr);

			    if(write(sockfd , sendBuff , MAXLINE) < 0)
			    {
				printf("Send failed\n");
				exit(1);     
			    } 

			    printf("size of sendBuff: %d\n", (int)strlen(sendBuff));

			    int recfd2;
			    char recvBuff[MAXLINE];
			    bzero(recvBuff, sizeof(recvBuff));
			    /*
			    while(1){

				if(recv(sockfd, recvBuff, MAXLINE, 0) <= 0){
					printf("Receive failed or done\n");
					break;
				}

				if(send(connfd , recvBuff , strlen(recvBuff) , 0) < 0){
					printf("Send failed\n");
					exit(1);     
				}
				printf("while loop success\n");	
				bzero(recvBuff, MAXLINE); 
			    }
			    */
			    
			    while( (recfd2 = read(sockfd , recvBuff , MAXLINE)) > 0)
			    {
				    printf("Recieved data length:%d  data: %s\n", recfd2,recvBuff);
				    if(write(connfd , recvBuff , MAXLINE) < 0)
				    {
					printf("Send failed\n");
					exit(1);     
				    }
				    bzero(recvBuff, MAXLINE);
			    }
			    printf("Done with recieving data with recvfd: %d\n", recfd2);
			    close(sockfd);
			}//end useless if statement
		    
		    }else if(strncmp(command, "HEAD", 4) == 0){
			printf("Came into HEAD part\n");
		    }else{
			//SEND: send message back to client of can't do intruction
			printf("Not a valid instruction/command: %s\n",command);
		    }

	 	    printf("trace 1\n");
		    bzero(recvline, MAXLINE); 
		}
	      }
	      printf("trace 3\n");
	      if (read_size == 0){
		printf ("Done receiving data \n");
		//Maybe this not needed?
	      }

	      if (read_size < 0){
		//SEND: Need to send message back to browser of: waited too long for browser message
		//					                       to reach proxy instead of breaking
		printf ("Recv has timed out\n");
	      }

	    //Now need to connect to different server and send data and wait for response
	    close(connfd);
  	//}//close pid == 0
	//else{
	//    printf("In parents process\n");
	//}
  }//close inifite while loop
}//close method

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
  readWriteServer(listenfd);
  return 0;
}   
