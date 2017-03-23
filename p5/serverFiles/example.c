#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

void handler(int s)
{
printf("hello\n");
}

int main(void)
{
	struct sigaction s1;
	memset(&s1, 0, sizeof(s1));
	s1.sa_handler = handler;

	sigaction(SIGINT, &s1, NULL);

	sleep(2);
	printf("world\n");
}
