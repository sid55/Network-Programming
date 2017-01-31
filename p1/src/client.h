#ifndef CLIENT_H
#define CLIENT_H

#define MAXLINE 1024

extern int     sockfd, n;
extern char    recvline[MAXLINE + 1], sendline[1024];
extern struct sockaddr_in servaddr;
extern int port;

void numArgs(int);
int createSocket();
void createServer(const char*,const char*);
void connectSocket(int);
void readWriteSocket(int);

#endif


