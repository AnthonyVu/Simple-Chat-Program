/*---------------------------------------------------------------------------------------
   --	SOURCE FILE:    server.c -   A simple multiplexed echo server using TCP
   --
   --	PROGRAM:		server.exe
   --
   --	FUNCTIONS:		static void SystemFatal(const char* );
   --                   void add_user(struct my_struct *s);
   --                   void delete_user(struct my_struct *user);
   --                   struct my_struct *find_user(int user_id);
   --
   --	DATE:			April 3, 2018
   --
   --
   --
   --	DESIGNERS:	   Anthony Vu
   --
   --	PROGRAMMER:		Anthony Vu
   --
   --	NOTES:
   --   Listens for connection requests from clients and accepts them. Receives messages
   --   from clients and echoes them out to all connected clients except the sender. Keeps a
   --   hashmap of all ip addresses connected. If a client sends -q, disconnect the client
   --   and remove their ipaddress from the hashmap.
   ---------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "uthash.h"

struct my_struct {
	int id;        // key
	char ip[255];
	UT_hash_handle hh; /* makes this structure hashable */
};

struct my_struct *users = NULL;

#define SERVER_TCP_PORT 7000
#define BUFLEN  80 
#define TRUE  1
#define LISTENQ 5
#define MAXLINE 4096

static void SystemFatal(const char*);
void add_user(struct my_struct *s);
void delete_user(struct my_struct *user);
struct my_struct *find_user(int user_id);

int main(int argc, char **argv)
{
	int i, maxi, nready, bytes_to_read, arg;
	int listen_sd, new_sd, sockfd, port, maxfd, client[FD_SETSIZE];
	unsigned int client_len;
	struct sockaddr_in server, client_addr;
	char *bp, buf[BUFLEN];
	ssize_t n;
	fd_set rset, allset;
	switch (argc)
	{
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		SystemFatal("Cannot Create Socket!");

	arg = 1;
	if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
		SystemFatal("setsockopt");

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
		SystemFatal("bind error");

	listen(listen_sd, LISTENQ);

	maxfd = listen_sd;
	maxi = -1;

	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;
	FD_ZERO(&allset);
	FD_SET(listen_sd, &allset);


	while (TRUE)
	{
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (nready <= 0) {
			break;
		}
		if (FD_ISSET(listen_sd, &rset))
		{
			client_len = sizeof(client_addr);
			if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, &client_len)) == -1)
				SystemFatal("accept error");

			printf(" Remote Address:  %s\n", inet_ntoa(client_addr.sin_addr));

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0)
				{
					client[i] = new_sd;
					struct my_struct *user = malloc(sizeof(struct my_struct));
					user->id = i;
					strcpy(user->ip, inet_ntoa(client_addr.sin_addr));
					add_user(user);
					break;
				}
			if (i == FD_SETSIZE)
			{
				printf("Too many clients\n");
				exit(1);
			}

			FD_SET(new_sd, &allset);
			if (new_sd > maxfd)
				maxfd = new_sd;

			if (i > maxi)
				maxi = i;

			if (--nready <= 0)
				continue;
		}

		for (i = 0; i <= maxi; i++)
		{
			if ((sockfd = client[i]) < 0)
				continue;

			if (FD_ISSET(sockfd, &rset))
			{
				struct my_struct *user = find_user(i);
				bp = buf;
				bytes_to_read = BUFLEN;

				while ((n = read(sockfd, bp, bytes_to_read)) > 0)
				{
					bp += n;
					bytes_to_read -= n;
					if (strcmp(buf, "-q\n") == 0) {
						printf(" closed %s\n", user->ip);
						client[i] = -1;
						delete_user(user);
						break;
					}
					else if (strcmp(buf, "-s\n") == 0) {
						continue;
					}
					for (int j = 0; j <= maxi; j++) {
						if ((sockfd = client[j]) < 0 || j == i)
							continue;
						char ip[BUFLEN];
						strcpy(ip, user->ip);
						strcat(ip, ": ");
						strcat(ip, buf);
						write(sockfd, ip, BUFLEN);
					}
				}

				if (nready-- <= 0) {
					break;
				}

			}
		}
	}
	return(0);
}

/*---------------------------------------------------------------------------------------
   --   FUNCTION:       add_user
   --	DATE:			April 3, 2018
   --
   --	REVISIONS:
   --
   --	DESIGNERS:		Troy D. Hanson
   --
   --	PROGRAMMERS:	Troy D. Hanson
   --
   --   INTERFACE:      void add_usert(my_struct *s)
   --                     s: user to add
   --   RETURNS:        VOID
   --
   --	NOTES:          Add user to hashmap
   --
   ---------------------------------------------------------------------------------------*/
void add_user(struct my_struct *s) {
	HASH_ADD_INT(users, id, s);
}

/*---------------------------------------------------------------------------------------
   --   FUNCTION:       delete_user
   --	DATE:			April 3, 2018
   --
   --	REVISIONS:
   --
   --	DESIGNERS:      Troy D. Hanson
   --
   --	PROGRAMMERS:    Troy D. Hanson
   --
   --   INTERFACE:      void delete_user(struct my_struct *user)
   --                     user: user to delete from the hashmap
   --   RETURNS:        void
   --
   --	NOTES:          Deletes user from the hashmap.
   --
   ---------------------------------------------------------------------------------------*/
void delete_user(struct my_struct *user) {
	HASH_DEL(users, user);
}

/*---------------------------------------------------------------------------------------
   --   FUNCTION:     find_user
   --	DATE:	      April 3, 2018
   --
   --	REVISIONS:
   --
   --	DESIGNERS:    Troy D. Hanson
   --
   --	PROGRAMMERS:  Troy D. Hanson
   --
   --   INTERFACE:    struct my_struct *find_user(int user_id)
   --                   user_id: the key of the user
   --   RETURNS:      struct
   --
   --	NOTES:        Finds and returns the user if the hashmap contains the user
   --
   ---------------------------------------------------------------------------------------*/
struct my_struct *find_user(int user_id) {
	struct my_struct *s;

	HASH_FIND_INT(users, &user_id, s);
	return s;
}

/*---------------------------------------------------------------------------------------
   --   FUNCTION:     SystemFatal
   --	DATE:         April 3, 2018
   --
   --	REVISIONS:
   --
   --	DESIGNERS:    Aman Abdulla
   --
   --	PROGRAMMERS:  Aman Abdulla
   --
   --   INTERFACE:    static void SystemFatal(const char* message)
   --                   message: error message
   --   RETURNS:      void
   --
   --	NOTES:        Prints out error messages
   --
   ---------------------------------------------------------------------------------------*/
static void SystemFatal(const char* message)
{
	perror(message);
	exit(EXIT_FAILURE);
}
