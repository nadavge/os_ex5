/*
 * clftp.cpp
 *
 * A file transfer protocol client
 *
 * Author: Daniel Danon
 *         Nadav Geva
 */

#include <iostream>
#include <climits>
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

#define ever (;;)

#define ARG_PORT 1
#define ARG_MAX_FILE_SIZE 2
#define NUM_ARGS 3

#define MIN_PORT 1
#define MAX_PORT 65535
#define MAX_CONNECTIONS 5

#define BUFFER_SIZE 1024

#define HERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, h_errno)
#define ERROR_MESSAGE(libraryName) GENERAL_ERROR_MESSAGE(libraryName, errno)
#define GENERAL_ERROR_MESSAGE(libraryName, errVar) cerr << "Error: function:" << libraryName << " errno:" << errVar << "." << endl
#define USAGE "Usage: srftp server-port max-file-size"

// ================================= GLOBALS ========================================= //

static int gMaxFileSize = -1;

// ================================= IMPLEMENTATION ================================== //

/**
* @brief Handles incoming connections
*
* @param connfd int* of the fd for the connection socket. Needs to be freed inside
*/
void* connectionHandler(void* connfd)
{
	int sockfd = *(int*)connfd;
	
	int fileSize = -1;
	bool fileSizeOk = false;

	FILE* file = nullptr;
	char fileName[NAME_MAX + 1] = {0};
	int fileNameLen = 0;

	int bytesRead = 0;
	int bytesWritten = 0;
	int totalBytes = 0;

	char buffer[BUFFER_SIZE] = {0};

	delete (int*)connfd; // Release memory as soon as possible to avoid memory leaks

	if ((bytesRead=recv(sockfd, &fileSize, sizeof(fileSize), 0)) < 0)
	{
		ERROR_MESSAGE("recv");
		goto error;
	}
	fileSize = ntohl(fileSize);
	
	fileSizeOk = fileSize <= gMaxFileSize;

	// We assume the size to be 1. If not one, might cause trouble
	if (send(sockfd, &fileSizeOk, sizeof(fileSizeOk), 0) < 0)
	{
		ERROR_MESSAGE("send");
		goto error;
	}

	if (! fileSizeOk)
	{
		goto finish;
	}

	if ((bytesRead = recv(sockfd, fileName, sizeof(fileName), 0)) <= 0)
	{
		ERROR_MESSAGE("recv");
		goto error;
	}
	
	fileNameLen = strlen(fileName);
	file = fopen(fileName, "wb");
	if (file == NULL)
	{
		ERROR_MESSAGE("fopen");
		goto error;
	}
	
	/*
	 * When we read into fileName, we might read more bytes than the file name, therefore
	 * we must copy the rest into the buffer to be put into the file
	 */
	
	// Set bytesRead to hold the amount of DATA bytes (exclude the file name from the message)
	bytesRead = bytesRead - (fileNameLen + 1);
	totalBytes = 0;
	memcpy(buffer, &fileName[fileNameLen+1], bytesRead);

	while(totalBytes < fileSize)
	{
		bytesWritten = 0;
		
		while (bytesWritten < bytesRead)
		{
			bytesWritten += fwrite(&buffer[bytesWritten], sizeof(char), bytesRead-bytesWritten, file);

			if (ferror(file))
			{
				ERROR_MESSAGE("fwrite");
			}
		}

		totalBytes += bytesWritten;

		bytesRead = recv(sockfd, buffer, sizeof(buffer), 0);

		/*
		 * Check if one of the following happened:
		 *  1. The connection ended before we finished reading the file
		 *  2. An error occurred on read
		 */
		if ((bytesRead == 0 && totalBytes < fileSize) || bytesRead < 0)
		{
			ERROR_MESSAGE("recv");
			goto error;
		}

	}

	goto finish;

error:
finish:
	if (file != nullptr)
	{
		fclose(file);
	}
	close(sockfd);

	return nullptr;
}

int main(int argc, char *argv[])
{
	int sockfd = -1;
	int tempConnfd = -1;
	int* connfd = nullptr;
	int port = -1;
	struct sockaddr_in servAddr = {0};
	struct sockaddr_in clientAddr = {0};
	socklen_t clientLen = 0;
	bool errorOccurred = false;
	pthread_t tid = 0;
	
	if (argc < NUM_ARGS) {
		cout << USAGE << endl;
		exit(1);
	}

	if (strToNum(argv[ARG_PORT], &port) == -1 || port < MIN_PORT || MAX_PORT < port)
	{
		cout << USAGE << endl;
		exit(1);
	}
	
	if (strToNum(argv[ARG_MAX_FILE_SIZE], &gMaxFileSize) == -1 || gMaxFileSize < 0)
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

		tempConnfd = accept(sockfd, (struct sockaddr*) &clientAddr, &clientLen);
		if (tempConnfd < 0)
		{
			delete connfd;

			ERROR_MESSAGE("accept");
			goto error;
		}

		/*
		 * Since we send the fd to a new thread, to avoid race conditions we must
		 * allocate a variable just for the specific thread.
		 */
		connfd = new(nothrow) int(0);
		if (connfd == nullptr)
		{
			ERROR_MESSAGE("new");
			goto error;
		}

		*connfd = tempConnfd;

		pthread_create(&tid, NULL, connectionHandler, connfd);
		pthread_detach(tid);

	}

	goto finish;

error:
	errorOccurred = true;

finish:
	close(sockfd);

	return errorOccurred ? 1 : 0;
}
