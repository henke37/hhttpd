#include "request.h"
#include "client.h"
#include "handler.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct request *newRequest(struct client *client) {
	struct request *request;

	request=(struct request *)malloc(sizeof(struct request));

	client->numRequests++;
	client->currentRequest=request;

	request->client=client;
	request->requestPath.buff=NULL;
	request->requestPath.allocLen=0;
	request->requestPath.usedLen=0;
	request->replyHeaders=NULL;
	request->requestBody=NULL;
	request->requestHeaders=NULL;
	request->status=REQUEST_LINE;
	request->isFileReply=0;
	request->replyBody=NULL;
	request->replyBodySize=0;
	request->replyCode=0;

	return request;
}

static void freeHeaderList(struct headerListNode *node) {
	struct headerListNode *headerToFree;

	while(node) {
		headerToFree=node;
		node=node->nextNode;

		free(headerToFree->name);
		free(headerToFree->value);
		free(headerToFree);
	}
}

void destroyRequest(struct request *request) {

	if(request==NULL) return;

	if(request->isFileReply) {
		closeNativeFile(request->replyFile);
	} else {
		free(request->replyBody);
	}

	freeHeaderList(request->replyHeaders);
	freeHeaderList(request->requestHeaders);

	free(request->requestBody);
	free(request->requestPath.buff);

	request->client->currentRequest=NULL;

	free(request);
}

static void parseRequestLine(struct request *);
static void parseRequestHeader(struct request *,char *nl);

void parseRequest(struct client *client) {
	struct request *request;
	int match=0;

	char *nl;

	size_t lineLength,newBuffLen;

	do {

		if(!client->currentRequest) {
			request=newRequest(client);
		} else {
			request=client->currentRequest;
		}

		match=0;
		switch(request->status) {
			case REQUEST_LINE:
			case HEADERS:
				nl=(char *)memchr((void *)client->readBuff,'\n',client->usedReadBuffLen);
				if(nl) {
					match=1;

					*nl=0;
					if(nl-1>=client->readBuff && *(nl-1)=='\r') {
						*(nl-1)=0;
					}

					if(request->status==REQUEST_LINE) {
						parseRequestLine(request);
						if(request->status!=INVALID) {
							request->status=HEADERS;
						}
					} else {
						if(nl-1==client->readBuff) {
							request->status=PARSED;
						} else {
							parseRequestHeader(request,nl);
						}
					}

					lineLength=nl - client->readBuff + 1;
					newBuffLen=client->usedReadBuffLen - lineLength;

					memmove(client->readBuff,nl+1, newBuffLen);
					client->usedReadBuffLen=newBuffLen;
				}
			break;

			case BODY:
			break;

			case PARSED:
			case INVALID:
			break;

		}

		if(request->status==PARSED || request->status==INVALID) {
			respondToRequest(request);
			destroyRequest(request);
			request=NULL;
		}

	} while(match && client->usedReadBuffLen);

}

char *requestMethodAsString(enum requestMethod requestMethod) {
	switch(requestMethod) {
	case GET:
		return "GET";
	case HEAD:
		return "HEAD";
	case POST:
		return "POST";
	case PUT:
		return "PUT";
	case DELETE:
		return "DELETE";
	case TRACE:
		return "TRACE";
	case OPTIONS:
		return "OPTIONS";
	case CONNECT:
		return "CONNECT";
	default:
		return "UNKNOWN";
	}
}

static void parseRequestLine(struct request *request) {
	char *firstSpace;
	char *secondSpace;
	char *versionDot;
	char *buff=request->client->readBuff;
	
	firstSpace=(char *)memchr(buff,' ',request->client->usedReadBuffLen);

	if(!firstSpace) {
		request->status=INVALID;
		return;
	}

	*firstSpace=0;
	if(strcmp("GET",buff)==0) {
		request->requestMethod=GET;
	} else if(strcmp("HEAD",buff)==0) {
		request->requestMethod=HEAD;
	} else if(strcmp("POST",buff)==0) {
		request->requestMethod=POST;
	} else if(strcmp("PUT",buff)==0) {
		request->requestMethod=PUT;
	} else if(strcmp("DELETE",buff)==0) {
		request->requestMethod=DELETE;
	} else if(strcmp("TRACE",buff)==0) {
		request->requestMethod=TRACE;
	} else if(strcmp("OPTIONS",buff)==0) {
		request->requestMethod=OPTIONS;
	} else if(strcmp("CONNECT",buff)==0) {
		request->requestMethod=CONNECT;
	} else {
		request->requestMethod=UNKNOWN;
	}

	secondSpace=strstr(firstSpace+1," ");

	if(!secondSpace) {
		request->status=INVALID;
		return;
	}

	*secondSpace=0;

	request->requestPath.usedLen=strlen(firstSpace+1)+1;
	request->requestPath.allocLen=request->requestPath.usedLen;

	request->requestPath.buff=(char *)malloc(request->requestPath.allocLen);
	strcpy(request->requestPath.buff,firstSpace+1);

	if(strncmp(secondSpace+1,"HTTP",4)!=0) {
		request->status=INVALID;
		return;
	}

	request->client->version.major=atoi(secondSpace+6);

	versionDot=strstr(secondSpace+1,".");

	if(!versionDot) {
		request->status=INVALID;
		return;
	}

	request->client->version.minor=atoi(versionDot+1);

}

static char *firstNonMatch(char *haystack,char needle) {
	for(;*haystack==0;++haystack) {
		if(*haystack==needle) {
			return haystack;
		}
	}
	return NULL;
}

static void parseRequestHeader(struct request *request,char *nl) {
	char *colon,*nonSpace;
	unsigned int len;

	struct headerListNode *header;

	colon=strstr(request->client->readBuff,":");

	if(!colon) {
		request->status=INVALID;
		return;
	}

	header=(struct headerListNode *)malloc(sizeof(struct headerListNode));
	header->nextNode=request->requestHeaders;
	request->requestHeaders=header;

	*colon=0;

	len=colon - request->client->readBuff + 1;
	header->name=(char *)malloc(sizeof(char)*len);
	strcpy(header->name,request->client->readBuff);

	nonSpace=firstNonMatch(colon+1,' ');

	if(!nonSpace) {
		nonSpace=colon+1;
	}

	len=nl - nonSpace+1;
	header->value=(char *)malloc(sizeof(char)*len);
	strcpy(header->value,nonSpace);
}

static int compareHeaderNames(const char *x,const char *y);

struct headerListNode *findHeader(struct headerListNode *node,const char *name) {

	for(;node;node=node->nextNode) {
		if(compareHeaderNames(node->name,name)==0) {
			return node;
		}
	}
	return NULL;
}

struct headerListNode *findRequestHeader(struct request *request,const char *name) {
	return findHeader(request->requestHeaders,name);
}

static int compareHeaderNames(const char *x,const char *y) {
	//TODO: use case and dash insensitive comparision
	return strcmp(x,y);
}
