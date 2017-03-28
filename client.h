#ifndef CLIENT_H
#define CLIENT_h

#include "socket.h"

struct request;

struct client {
	struct request *currentRequest;
	socket_t socket;
	char *readBuff;
	size_t readBuffLen, usedReadBuffLen;

	struct sockaddr_in addr;

	unsigned int numRequests;

	unsigned int id;

	struct {
		short major, minor;
	} version;
};

struct client *newClient(socket_t listenSock);
void readFromClient(struct client *client);
void destroyClient(struct client *client);

struct clientList {
	struct client **clients;
	unsigned int used,allocated;
};
extern struct clientList clientList;

#endif