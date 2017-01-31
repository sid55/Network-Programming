#ifndef SERVER_H
#define SERVER_H

#define MAXLINE 4096
#define LISTENQ 1024

extern int     listenfd, connfd,read_size;
extern struct sockaddr_in servaddr;
extern char    buff[1025];
extern time_t ticks;
extern int port;
extern FILE *in;
extern FILE *popen();

void numArgs(int);
int createListenSocket();
void initializeServer(const char*);
void bindServer(int);
void listenServer(int);
int acceptServer(int);
void readWriteServer(int,int);
void acceptReadWriteServer(int);

#endif
