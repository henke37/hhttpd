#include "socket.h"
#include "client.h"

#include <stdlib.h>
#include <stdio.h>

#define BASE_CLIENTS 10

void listenForConnections(unsigned short int port) {

	socket_t serverSocket;
	struct client *client;

	int maxSock=0;

	socket_t s;

	unsigned int i;

	fd_set readSocks;

	clientList.clients=(struct client **)calloc(BASE_CLIENTS,sizeof(struct client *));
	clientList.used=0;
	clientList.allocated=BASE_CLIENTS;

	serverSocket=createListeningSocket(port);

	if(!isValidSocket(serverSocket)) {
		printf("Could not listen to port %d\n",port);
		return;
	}

	puts("Server now listening");

	for(;;) {

#ifdef USE_BERKELY
		maxSock=serverSocket;
#endif

		FD_ZERO(&readSocks);
		for(i=0;i<clientList.used;++i) {
			s=clientList.clients[i]->socket;
			FD_SET(s,&readSocks);
#ifdef USE_BERKELY
			if(maxSock<s) {
				maxSock=s;
			}
#endif
		}
		FD_SET(serverSocket,&readSocks);

		select(maxSock+1,&readSocks,NULL,NULL,NULL);

		if(FD_ISSET(serverSocket,&readSocks)) {
			client=newClient(serverSocket);
		}

		for(i=0;i<clientList.used;++i) {
			if(FD_ISSET(clientList.clients[i]->socket,&readSocks)) {
				readFromClient(clientList.clients[i]);
			}
		}
	}
}
