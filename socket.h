#ifndef SOCKET_H
#define SOCKET_H

#include "nativeFile.h"

#ifdef WIN32 
#define USE_WINSOCK
#else
#define USE_BERKELY
#endif

#ifdef USE_WINSOCK
#include <WinSock2.h>
#include <inaddr.h>

struct socketData {
	WSADATA wsaData;
};

typedef SOCKET socket_t;

extern struct socketData socketData;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef int socket_t;

#define SD_BOTH 3
#define INVALID_SOCKET -1

#endif

int initSockets();
socket_t createListeningSocket(unsigned short port);
void closeListeningSocket(socket_t);
void deinitSockets();
int isValidSocket(socket_t);
void closeSocket(socket_t);
void sendNativeFile(file_t,socket_t);


#endif
