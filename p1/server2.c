#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
//#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define MAXLINE 4096
#define LISTENQ 1024

int     listenfd, connfd,read_size;
struct sockaddr_in servaddr;
char    buff[1025];
time_t ticks;
int port;
FILE *in;
extern FILE *popen();

void numArgs(int argc){
    if (argc != 2){
        perror("usage: a.out <PORTnumber>\n");
        exit(1);
    }
}

int createListenSocket(){
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0){
        perror("socket error\n");
        exit(1);
    }
    return listenfd;
}

void createServer(const char *portnum){
    sscanf(portnum,"%d",&port);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port); /* daytime server */
}

void bindServer(int listenfd){
    int bindID = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (bindID < 0){
        perror("bind error\n");
        exit(1);
    } 
}

void listenServer(int listenfd){
    int listenID = listen(listenfd, LISTENQ);
    if (listenID < 0){
        perror("listen error\n");
        exit(1);
    } 
}

int acceptServer(int listenfd){
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
        if (connfd < 0){
            perror("accept error\n");
            exit(1);
        } 
        return connfd;
}

void readWriteServer(int connfd, int listenfd){
  while( (read_size = recv(connfd , buff , 1024 , 0)) > 0 )
    {
        //Send the message back to client
        //char 

        if (!(in = popen(buff, "r"))) {
          perror("Not able to read stream");
          exit(1);
        } 

        //int length = (int)strlen(buff);
        //printf("gets before while loop %d\n",length);

        if(fgets(buff,sizeof(buff), in) == NULL){
            printf("success in finding problem");
            sprintf(buff,"empty");
            write(connfd, buff, strlen(buff));
            bzero(buff,1024);
        }else{
            printf("Gets into if statement\n");
            printf("%s", buff);
            write(connfd , buff , strlen(buff));
            bzero(buff,1024);
        }


        while (fgets(buff, sizeof(buff), in) != NULL) {
            printf("Gets into fgets\n");
            printf("%s", buff);
            write(connfd , buff , strlen(buff));
            bzero(buff,1024);
        }
        if(fgets(buff,sizeof(buff), in) == NULL){
            printf("success in finding problem TWO");
            sprintf(buff,"empty");
            write(connfd, buff, strlen(buff));
            bzero(buff,1024);
        }
        //int length = (int)strlen(buff);
        //printf("gets outside while loop %d\n",length);
        pclose(in);

        printf("\n------------------------\n");
        //printf(buff);

        //write(connfd , buff , strlen(buff));
        bzero(buff,1024);
    }
     
    if(read_size == 0)
    {
        perror("Client disconnected\n");
        close(connfd);
        //close(listenfd);
        //listenServer(listenfd);
        //fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed on server\n");
    }
}

void acceptReadWriteServer(int listenfd){
  while(1){
    int connfd = acceptServer(listenfd);
    readWriteServer(connfd, listenfd);
  }
}

int
main(int argc, char **argv)
{
  numArgs(argc);
  int listenfd = createListenSocket();
  createServer(argv[1]);
  bindServer(listenfd);
  listenServer(listenfd);

  //while(1){
  //int connfd = acceptServer(listenfd);
  //readWriteServer(connfd, listenfd);
  //}

  acceptReadWriteServer(listenfd);
  return 0;
}
