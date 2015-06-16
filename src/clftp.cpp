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
#define GENERAL_ERROR_MESSAGE(libraryName, errVar) cerr << "Error: function:" << libraryName << " errno:" << errVar << "." << endl
#define USAGE "Usage: clftp server-port server-hostname file-to-transfer filename-in-server"
#define MSG_FILE_TOO_BIG "Transmission failed: too big file"

// ================================= IMPLEMENTATION ================================== //

//TODO Document
int sendBuffer (int sockfd, char* buffer, int bufferSize){
	int bytesSent = 0;
	int sent = -1;

	while (bytesSent < bufferSize)
	{
		sent = send(sockfd, buffer+bytesSent, bufferSize-bytesSent, 0);
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
	int fileSize = 0;
	FILE* file = nullptr;
	bool errorOccurred = false;
	
	bool fileSizeOk = false;
	int bytesRead = 0;
	int totalBytes = 0;
	
	if (argc < NUM_ARGS) {
		cout << USAGE << endl;
		goto error;
	}

	if (strToNum(argv[ARG_PORT], &port) == -1 || port < MIN_PORT || MAX_PORT < port)
	{
		cout << USAGE << endl;
		goto error;
	}
	
	file = fopen(argv[ARG_FILE_LOCAL], "rb");
	if (file == nullptr)
	{
		// TODO check if to print - forum
		cout << USAGE << endl;
		goto error;
	}

	if(fstat(fileno(file), &fileStat) < 0)
	{
		ERROR_MESSAGE("fstat");
		goto error;
	}

	fileSize = fileStat.st_size; // In bytes
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		ERROR_MESSAGE("socket");
		goto error;
	}

	server = gethostbyname(argv[ARG_HOSTNAME]);
	if (server == nullptr) {
		HERROR_MESSAGE("gethostbyname");
		goto error;
	}

	servAddr.sin_family = AF_INET;
	memcpy((char *)&servAddr.sin_addr.s_addr,
		   (char *)server->h_addr,
		   server->h_length);
	servAddr.sin_port = htons(port);
	
	if (connect(sockfd,(struct sockaddr *) &servAddr,sizeof(servAddr)) < 0)
	{
		ERROR_MESSAGE("connect");
		goto error;
	}

	// Convert to network byte-order - if ever used again, remember to convert back
	fileSize = htonl(fileSize);
	if (send(sockfd, &fileSize, sizeof(fileSize), 0) < 0)
	{
		ERROR_MESSAGE("send");
		goto error;
	}
	fileSize = ntohl(fileSize);

	if (recv(sockfd, &fileSizeOk, sizeof(fileSizeOk), 0) < 0)
	{
		ERROR_MESSAGE("recv");
		goto error;
	}

	if (! fileSizeOk)
	{
		cout << MSG_FILE_TOO_BIG << endl;
		goto finish;
	}

	// Send name of file on server
	if (sendBuffer(sockfd, argv[ARG_FILE_SERVER], (strlen(argv[ARG_FILE_SERVER])+1)*sizeof(char)) < 0)
	{
		ERROR_MESSAGE("send");
		goto error;
	}

	totalBytes = 0;
	while(totalBytes < fileSize)
	{
		bytesRead = fread(gBuffer, sizeof(char), BUFFER_SIZE, file);
	
		if (ferror(file))
		{
			ERROR_MESSAGE("fread");
			goto error;
		}
		
		if (sendBuffer(sockfd, gBuffer, bytesRead) < 0)
		{
			ERROR_MESSAGE("send");
			goto error;		
		}

		totalBytes += bytesRead;
	}

	goto finish;
error:
	errorOccurred = true;

finish:
	if (file != nullptr)
	{
		fclose(file);
	}
	if (sockfd >= 0)
	{
		close(sockfd);
	}

	return errorOccurred ? 1 : 0;
}
