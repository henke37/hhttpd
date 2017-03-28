#include "reply.h"
#include "request.h"
#include "client.h"
#include "socket.h"

#include <stdio.h>
#include <stdlib.h>

char *intToString(unsigned int);

#include "knownString.h"

static struct knownString buildReplyLine(struct request *);

void sendReply(struct request *request) {
	struct headerListNode *header;

	socket_t s=request->client->socket;

	struct knownString replyLine;

	char *tempStr;

	replyLine=buildReplyLine(request);
	send(s,replyLine.buff,replyLine.usedLen,0);
	free(replyLine.buff);

	if(request->isFileReply) {
		request->replyBodySize=sizeOfNativeFile(request->replyFile);
	}

	if(request->replyBodySize) {
		tempStr=intToString(request->replyBodySize);
		setReplyHeader(request,"Content-Length",tempStr);
		free(tempStr);
	}

	for(header=request->replyHeaders;header;header=header->nextNode) {
		send(s,header->name,strlen(header->name),0);
		send(s,": ",2,0);
		send(s,header->value,strlen(header->value),0);
		send(s,"\r\n",2,0);
	}
	send(s,"\r\n",2,0);

	if(request->requestMethod==HEAD || request->replyCode==204 || request->replyCode==304) {
		return;
	}

	if(request->isFileReply) {
		sendNativeFile(request->replyFile,s);
	} else if(request->replyBodySize) {
		send(s,request->replyBody,request->replyBodySize,0);
	}
}

char const* replyDescFromCode(unsigned int code) {
	switch(code) {
		case 200:
			return "OK";
		break;

		case 301:
			return "Moved Permanently";

		case 304:
			return "Not modified";
		break;

		case 303:
			return "See Other";

		case 307:
			return "Temporary Redirect";

		case 400:
			return "Bad Request";
		break;

		case 404:
			return "Not found";
		break;

		case 403:
			return "Forbidden";
		break;

		case 410:
			return "Gone";
		break;

		case 414:
			return "Request-URI Too Long";

		case 431:
			return "Request Header Fields Too Large";

		case 417:
			return "Expectation Failed";
		break;

		case 500:
			return "Internal error";
		break;

		case 501:
			return "Not implemented";
		break;

		case 505:
			return "HTTP Version Not Supported";
		break;

		default:
			return "UNKNOWN";
		break;

	}
}

char *intToString(unsigned int v) {
	char *buff;

	buff=(char*)malloc(20);
#ifdef WIN32
	_itoa_s(v,buff,20,10);
#endif

	return buff;
}

struct headerListNode *findHeader(struct headerListNode *node,const char *name);

struct headerListNode *findReplyHeader(struct request *request,const char *name) {
	return findHeader(request->replyHeaders,name);
}

void setReplyHeader(struct request *request,char *name,char *value) {

	struct headerListNode *header;

	header=findReplyHeader(request,name);

	if(!header) {
		header=(struct headerListNode *)malloc(sizeof(struct headerListNode));
		header->nextNode=request->replyHeaders;
		request->replyHeaders=header;

		header->name=(char *)malloc(strlen(name)+1);
		strcpy(header->name,name);
	} else {
		free(header->value);
	}

	header->value=(char *)malloc(strlen(value)+1);
	strcpy(header->value,value);
}

static struct knownString buildReplyLine(struct request *request) {

	struct knownString line;

	const char *desc;
	size_t descLen;

	line.buff=(char *)malloc(35);
	line.allocLen=35;
	line.usedLen=0;

	strcpy(line.buff,"HTTP/");
	line.usedLen=5;

	line.usedLen+=sprintf(
		&line.buff[line.usedLen],
		"%d.%d %d ",
		request->client->version.major,
		request->client->version.minor,
		request->replyCode
	);

	desc=replyDescFromCode(request->replyCode);
	descLen=strlen(desc);
	if(descLen+2+(line.allocLen-line.usedLen)>line.allocLen) {
		line.allocLen+=descLen+2;
		line.buff=(char *)realloc(line.buff,line.allocLen);
	}

	strcpy(&line.buff[line.usedLen],desc);
	line.usedLen+=descLen;

	strcpy(&line.buff[line.usedLen],"\r\n");
	line.usedLen+=2;

	return line;
}