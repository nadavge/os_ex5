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

#define ever (;;)

#define ARG_PORT 1
#define ARG_MAX_FILE_SIZE 2
#define NUM_ARGS 3

#define MIN_PORT 1
#define MAX_PORT 65535
#define MAX_CONNECTIONS 5

#define BUFFER_SIZE 1024

static char gBuffer[BUFFER_SIZE] = {0};

#define HERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, h_errno)
#define ERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, errno)
#define GENERAL_ERROR_MESSAGE(libraryName, errVar) cerr << "Error: function:" << libraryName << "errno:" << errVar << "." << endl
#define USAGE "Usage: srftp server-port max-file-size"

// ================================= IMPLEMENTATION ================================== //

//TODO Document/REMOVE
int sendBuffer (int sockfd, int bufferSize){
	int bytesSent = 0;
	int sent = -1;

	while (bytesSent < bufferSize)
	{
		sent = send(sockfd, gBuffer+bytesSent, bufferSize-bytesSent, 0);
		if (sent < 0)
		{
			return -1;
		}

		bytesSent += sent;
	}

	return bytesSent;
}

//TODO  Reuse for ftp
/*
int magicFunction()
{
	if (send(sockfd, &fileSize, sizeof(fileSize), 0) < 0)
	{
		ERROR_MESSAGE("send");
		goto error;
	}

	if (recv(sockfd, &fileSizeOk, sizeof(fileSizeOk), 0) < 0)
	{
		ERROR_MESSAGE("recv");
		goto error;
	}

	if (send(sockfd, argv[ARG_FILE_SERVER], strlen(argv[ARG_FILE_SERVER])*sizeof(char), 0) < 0)
	{
		ERROR_MESSAGE("send");
		goto error;
	}

	while ((bytesRead=fread(gBuffer, BUFFER_SIZE, 1, file)) > 0)
	{
		if (sendBuffer(sockfd, bytesRead) < 0)
		{
			ERROR_MESSAGE("send");
			goto error;		
		}
	}

	// If while ended before EOF
	if (! feof(file))
	{
		ERROR_MESSAGE("fread");
		goto error;
	}
}*/

int main(int argc, char *argv[])
{
	int sockfd = -1;
	int connSocket = -1;
	int port = -1;
	int maxFileSize = -1;
	struct sockaddr_in servAddr = {0};
	struct sockaddr_in clientAddr = {0};
	socklen_t clientLen = 0;
	bool errorOccurred = false;
	
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
	
	maxFileSize = strtol(argv[ARG_PORT], nullptr, 10);
	if (maxFileSize < 0)
	{
		cout << USAGE << endl;
		exit(1);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		ERROR_MESSAGE("socket");
		goto error;
	}

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
	{
		ERROR_MESSAGE("bind");
		goto error;
	}

	if (listen(sockfd, MAX_CONNECTIONS) < 0)
	{
		ERROR_MESSAGE("listen");
		goto error;
	}

	clientLen = sizeof(clientAddr);
	
	for ever // It's a joke, don't kill us :(
	{
		connSocket = accept(sockfd, (struct sockaddr*) &clientAddr, &clientLen);
		if (connSocket < 0)
		{
			ERROR_MESSAGE("accept");
			goto error;
		}
		//TODO initiate new thread
	}

	goto finish;

error:
	errorOccurred = true;

//TODO match for server
finish:
	close(sockfd);

	return errorOccurred ? 1 : 0;
}
