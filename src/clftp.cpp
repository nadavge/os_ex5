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
#include "utils.h"

using namespace std;

// ================================= CONSTANTS ======================================= //

#define ARG_PORT 1
#define ARG_HOSTNAME 2
#define ARG_FILE_LOCAL 3
#define ARG_FILE_SERVER 4
#define NUM_ARGS 5

#define MIN_PORT 1
#define MAX_PORT 65535

#define BUFFER_SIZE 1024

static char gBuffer[BUFFER_SIZE] = {0};

#define HERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, h_errno)
#define ERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, errno)
#define GENERAL_ERROR_MESSAGE(libraryName, errVar) cerr << "Error: function:" << libraryName << "errno:" << errVar << "." << endl
#define USAGE "Usage: clftp server-port server-hostname file-to-transfer filename-in-server"

// ================================= IMPLEMENTATION ================================== //

//TODO Document
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

int main(int argc, char *argv[])
{
	int sockfd = -1;
	int port = -1;
	struct sockaddr_in servAddr = {0};
	struct hostent *server = nullptr;
	struct stat fileStat = {0};
	off_t fileSize = 0;
	FILE* file = nullptr;
	bool errorOccurred = false;
	
	bool fileSizeOk = false;
	int bytesRead = 0;
	
	if (argc < NUM_ARGS) {
		cout << USAGE << endl;
		exit(1);
	}

	if (strToNum(argv[ARG_PORT], &port) == -1 || port < MIN_PORT || MAX_PORT < port)
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
	memcpy((char *)&servAddr.sin_addr.s_addr,
		   (char *)server->h_addr,
		   server->h_length);
	servAddr.sin_port = htons(port);
	
	if (connect(sockfd,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)
	{
		fclose(file);
		ERROR_MESSAGE("connect");
		exit(1);
	}

	// Convert to network byte-order - if ever used again, remember to convert back
	// TODO check we don't use it again (Do only after ex is complete)
	fileSize = htonl(fileSize);
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

	while(feof(file) != 0)
	{
		bytesRead = fread(gBuffer, sizeof(char), BUFFER_SIZE, file);
		
		if (ferror(file) != 0)
		{
			ERROR_MESSAGE("fread");
			goto error;
		}

		if (sendBuffer(sockfd, bytesRead) < 0)
		{
			ERROR_MESSAGE("send");
			goto error;		
		}
	}

	goto finish;

error:
	errorOccurred = true;

finish:
	fclose(file);
	close(sockfd);

	return errorOccurred ? 1 : 0;
}
