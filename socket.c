#include "socket.h"

#include <stdlib.h>

#ifdef WIN32
#include <Mswsock.h>
#else
#include <sys/sendfile.h>
#include <unistd.h>
#endif

#ifdef USE_WINSOCK

struct socketData socketData;

int initSockets() {
	WORD versionRequested;
	int err;

	versionRequested = MAKEWORD(2, 2);

	err=WSAStartup(versionRequested, &socketData.wsaData);
	if(err != 0) { return 0; }

	if(LOBYTE(socketData.wsaData.wVersion) != 2 || HIBYTE(socketData.wsaData.wVersion) != 2) {
		WSACleanup();
		return 0;
	}

	return 1;

};

void deinitSockets() {
	WSACleanup();
};

void closeSocket(socket_t s) {
	closesocket(s);
}

void sendNativeFile(file_t f,socket_t s) {
	TransmitFile(s,f,0,0,NULL,NULL,0);
}

#else

int initSockets() {
	return 1;
}

void deinitSockets() {}

void closeSocket(socket_t s) {
	close(s);
}

void sendNativeFile(file_t f,socket_t s) {

	int remaining=sizeOfNativeFile(f);
	int result;

	do {
		result=sendfile(s,f,NULL,remaining);

		if(result==-1) return;
		remaining-=result;
	} while(remaining>0);
}

#endif

socket_t createListeningSocket(unsigned short port) {
	socket_t s;
	struct sockaddr_in addr;
	int err;

	s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if(s==INVALID_SOCKET) {
		return s;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
#ifdef WIN32
	err=bind(s,(SOCKADDR*) &addr,sizeof(addr));
#else
	err=bind(s,(struct sockaddr*) &addr,sizeof(addr));
#endif
	if(err!=0) {
		closeSocket(s);
		return INVALID_SOCKET;
	}

	err=listen(s,10);

	if(err!=0) {
		closeSocket(s);
		return INVALID_SOCKET;
	}

	return s;
}

int isValidSocket(socket_t s) { 
	return s!=INVALID_SOCKET;
};

