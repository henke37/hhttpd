#include "client.h"
#include "request.h"

#include <stdlib.h>
#include <stdio.h>

struct clientList clientList;

static unsigned int nextClientID=0;

struct client *newClient(socket_t s) {
	struct client *c;
	int addrSize=sizeof(c->addr);
	char *ip;

	c=(struct client *)malloc(sizeof(struct client));

	c->socket=accept(s,(struct sockaddr *)&c->addr,&addrSize);

	if(!isValidSocket(c->socket)) {
		free(c);
		return NULL;
	}

	c->currentRequest=NULL;
	c->readBuff=NULL;
	c->readBuffLen=c->usedReadBuffLen=0;

	c->numRequests=0;

	c->version.major=1;
	c->version.minor=0;

	c->id=nextClientID++;

	clientList.clients[clientList.used++]=c;

	ip=inet_ntoa(c->addr.sin_addr);

	printf("new client %d %s\n",c->id, ip);

	return c;
};

void destroyClient(struct client *client) {
	unsigned int i;

	for(i=0;i<clientList.used;++i) {
		if(clientList.clients[i]==client) {
			break;
		}
	}

	for(;i<clientList.used-1;++i) {
		clientList.clients[i]=clientList.clients[i+1];
	}
	clientList.used--;

	free(client->readBuff);
	closeSocket(client->socket);

	destroyRequest(client->currentRequest);

	if(clientList.used==0) {
		printf("free client %d (last). Next Id %d\n",client->id,nextClientID);
	} else {
		printf("free client %d\n",client->id);
	}

	free(client);
}

void readFromClient(struct client *client) {
	const size_t readLen=200;
	char *newBuff;
	size_t newSize;

	int readRes;
	
	if(!client->readBuff) {
		client->readBuff=(char *)malloc(sizeof(char)*readLen);
		client->readBuffLen=readLen;
	} else if(client->readBuffLen-client->usedReadBuffLen < readLen) {
		newSize=client->readBuffLen+readLen;

		newBuff=(char*)realloc(client->readBuff,newSize);
		client->readBuffLen=newSize;
		client->readBuff=newBuff;
	}

	readRes=recv(client->socket,client->readBuff+client->usedReadBuffLen,readLen,0);

	if(readRes <=0) {
		destroyClient(client);
		return;
	}

	client->usedReadBuffLen+=readRes;

	parseRequest(client);
};
