/*---------------------------------------------------------------------------------------
   --	SOURCE FILE:		client.c - A simple TCP client program.
   --
   --	PROGRAM:		client.exe
   --
   --	FUNCTIONS:		void* outputMsg
   --
   --	DATE:			April 3, 2018
   --
   --	REVISIONS:
   --
   --
   --	DESIGNERS:		Morgan Ariss
   --
   --	PROGRAMMERS:		Morgan Ariss
   --
   --	NOTES:
   --	The user specifies a port to connect to. Once a connection has been established, the
   --   user may begin sending data to the server. The messages are echoed to all other connected
   --   clients. The user may choose to close the connection by entering -q or -s to save a chat
   --   log.
   ---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#define SERVER_TCP_PORT  7000
#define BUFLEN   80

int sd, bytes_to_read;
char *bp, rbuf[BUFLEN];

void* outputMsg();
char savedBuffer[1000];
int i;
int main(int argc, char **argv)
{
	pthread_t thread1;
	int port, j;
	struct hostent *hp;
	struct sockaddr_in server;
	char  *host, sbuf[BUFLEN], **pptr;
	char str[16];
	i = 0;
	switch (argc)
	{
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(1);
	}
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if ((hp = gethostbyname(host)) == NULL)
	{
		fprintf(stderr, "Unknown server address\n");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect to server\n");
		perror("connect");
		exit(1);
	}
	printf("Connected:    Server Name: %s\n", hp->h_name);
	pptr = hp->h_addr_list;
	printf("\t\tIP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));
	printf("Press -q to quit | Press -s to save a log\n");
	pthread_create(&thread1, NULL, outputMsg, NULL);

	while (1) {
		FILE *fp;
		j = 0;
		fflush(stdin);
		fgets(sbuf, BUFLEN, stdin);
		while (sbuf[j] != '\0') {
			savedBuffer[i] = sbuf[j];
			i++;
			j++;
		}
		send(sd, sbuf, BUFLEN, 0);
		if (strcmp(sbuf, "-q\n") == 0) {
			close(sd);
			return 0;
		}
		else if (strcmp(sbuf, "-s\n") == 0) {
			fp = fopen("log.txt", "wb");
			fwrite(savedBuffer, strlen(savedBuffer), 1, fp);
			fclose(fp);
			printf("saved log\n");
		}
		memset(sbuf, 0, BUFLEN);
	}
	return (0);
}

/*---------------------------------------------------------------------------------------
   --   FUNCTION:       outputMsg
   --	DATE:			April 3, 2018
   --
   --	REVISIONS:
   --
   --
   --	DESIGNERS:		Morgan Ariss
   --
   --	PROGRAMMERS:	Morgan Ariss
   --
   --   INTERFACE:      void* outputMsg
   --
   --   RETURNS:        VOID
   --
   --	NOTES:          Output messsages received from other clients.
   --
   ---------------------------------------------------------------------------------------*/
void* outputMsg()
{
	int n, j;
	while (1) {
		bp = rbuf;
		bytes_to_read = BUFLEN;

		n = 0;
		j = 0;
		while ((n = recv(sd, bp, bytes_to_read, 0)) < BUFLEN)
		{
			bp += n;
			bytes_to_read -= n;
		}
		while (rbuf[j] != '\0') {
			savedBuffer[i] = rbuf[j];
			i++;
			j++;
		}
		printf("%s", rbuf);
		fflush(stdout);
		memset(bp, 0, BUFLEN);
	}
	return NULL;
}
