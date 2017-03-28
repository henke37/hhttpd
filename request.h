#ifndef REQUEST_H
#define REQUEST_H

#include "nativeFile.h"
#include "knownString.h"

#undef DELETE

enum requestStatus { REQUEST_LINE, HEADERS, BODY, PARSED, INVALID };

enum requestMethod { GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS, CONNECT, UNKNOWN=99 };

struct client;

struct headerListNode {
	struct headerListNode *nextNode;
	char *name;
	char *value;
};

struct request {
	enum requestStatus status;
	struct client *client;

	struct knownString requestPath;
	enum requestMethod requestMethod;
	struct headerListNode *requestHeaders;

	char *requestBody;

	unsigned short replyCode;
	struct headerListNode *replyHeaders;

	int isFileReply;
	union {
		file_t replyFile;
		char *replyBody;
	};
	size_t replyBodySize;
};

struct request *newRequest(struct client *);
void destroyRequest(struct request *);

void parseRequest(struct client *);

char *requestMethodAsString(enum requestMethod requestMethod);

struct headerListNode *findRequestHeader(struct request *request,const char *name);

#endif