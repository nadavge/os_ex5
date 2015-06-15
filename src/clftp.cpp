/*
 * clftp.cpp
 *
 * A file transfer protocol client
 *
 * Author: Daniel Danon
 *         Nadav Geva
 */

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

// ================================= CONSTANTS ======================================= //

#define ARG_PORT 1
#define ARG_HOSTNAME 2
#define ARG_FILE_LOCAL 3
#define ARG_FILE_DEST 4
#define NUM_ARGS 5

#define MIN_PORT 1
#define MAX_PORT 65535

#define HERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, h_errno)
#define ERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, errno)
#define GENERAL_ERROR_MESSAGE(libraryName, errVar) cerr << "Error: function:" << libraryName << "errno:" << errVar << "." << endl
#define USAGE "Usage: clftp server-port server-hostname file-to-transfer filename-in-server"

// ================================= IMPLEMENTATION ================================== //

int main(int argc, char *argv[])
{
	int sockfd = -1;
	int port = -1;
	int n = -1;
	struct sockaddr_in servAddr = {0};
	struct hostent *server = nullptr;
	struct stat fileStat = {0};
	off_t fileSize = 0;
	FILE* file = nullptr;
	if (argc < NUM_ARGS) {
		cout << USAGE << endl;
		exit(1);
	}


	port = strtol(argv[ARG_PORT], nullptr, 10);
	if (port < MIN_PORT || MAX_PORT < port)
	{
		cout << USAGE << endl;
		exit(1);
	}
	
	file = fopen(argv[ARG_FILE_LOCAL], "rb");
	if (file == nullptr)
	{
		cout << USAGE << endl;
		exit(1);
	}

	if(fstat(fileno(file), &fileStat) < 0)
	{
		fclose(file);
		ERROR_MESSAGE("fstat");
		exit(1);
	}

	fileSize = fileStat.st_size; // In bytes
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		fclose(file);
		ERROR_MESSAGE("socket");
		exit(1);
	}

	server = gethostbyname(argv[ARG_HOSTNAME]);
	if (server == nullptr) {
		fclose(file);
		HERROR_MESSAGE("gethostbyname");
		exit(1);
	}

	servAddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
		  (char *)&servAddr.sin_addr.s_addr,
		  server->h_length);
	servAddr.sin_port = htons(port);
	
	if (connect(sockfd,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)
	{
		fclose(file);
		ERROR_MESSAGE("connect");
		exit(1);
	}

/*	
	n = write(sockfd,buffer,strlen(buffer));
	if (n < 0)
		error("ERROR writing to socket");
	bzero(buffer,256);
	n = read(sockfd,buffer,255);
	if (n < 0)
		error("ERROR reading from socket");
	printf("%s\n",buffer);
*/
	fclose(file);
	close(sockfd);

	return 0;
}
