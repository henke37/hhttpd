#include "handler.h"
#include "request.h"
#include "client.h"
#include "reply.h"
#include "nativeFile.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "knownString.h"

static void setupErrorPage(struct request *request, unsigned int replyCode);

void handleRequest(struct request *);

static void setLastModifiedHeader(struct request *);
static void setDateHeader(struct request *);

static int checkIfModifiedSince(struct request * request);

static time_t parseHeaderTimeValue(const char *);
char *pathToFile(char *path,const char *baseDir);

char * strrstr(char *buff,char *needle);

static void guessMime(struct request *);

void respondToRequest(struct request *request) {

	struct headerListNode *header;
	int close;


	header=findRequestHeader(request,"Connection");

	if(header && strcmp(header->value,"close")==0) {
		close=1;
	} else {
		close=request->client->version.major<1 || request->client->version.minor < 1;
	}

	if(request->status==INVALID) {
		close=1;
	}

	if(close) {
		setReplyHeader(request,"Connection","close");
	}

	setReplyHeader(request,"Server","Henke37 Httpd");

	setDateHeader(request);

	handleRequest(request);

	sendReply(request);	

	if(close) {
		shutdown(request->client->socket,SD_BOTH);
	}
	
	printf("%d %s %s %hu\n",
		request->client->id,
		requestMethodAsString(request->requestMethod),
		request->requestPath.buff,
		request->replyCode
	);
}

char *intToString(unsigned int);

static void setupErrorPage(struct request *request, unsigned int replyCode) {

	file_t errorFile;
	char errorPath[20];
	const char *errorDescription;
	char *errorPageContent;
	char *errorFileName;

	size_t totalLength,descriptionLength,codeLength;

	request->replyCode=replyCode;

	sprintf(errorPath,"%u.html",replyCode);

	errorFileName=pathToFile(errorPath,"errors/");

	errorFile=openNativeFile(errorFileName,1);

	if(isValidNativeFile(errorFile)) {
		request->isFileReply=1;
		request->replyFile=errorFile;
	} else {
		errorDescription=replyDescFromCode(replyCode);

		descriptionLength=strlen(errorDescription);
		codeLength=3;

		totalLength=descriptionLength+1+codeLength;

		errorPageContent=(char *)malloc(totalLength+1);

		sprintf(errorPageContent,"%d %s",replyCode,errorDescription);

		setReplyHeader(request,"Content-type","text/plain");

		request->replyBody=errorPageContent;
		request->replyBodySize=totalLength;
	}

}

static void convertSlashes(char *);

void handleRequest(struct request *request) {
	struct headerListNode *header;
	char *badChar;

	char *fileName=NULL;
	file_t replyFile;

	const char *indexFile="index.html";

	if(request->status==INVALID) {
		if(request->replyCode) {
			setupErrorPage(request,request->replyCode);
		} else {
			setupErrorPage(request,400);
		}
		goto handleRequestCleanup;
	}

	if(request->client->version.major!=1 || request->client->version.minor > 1) {
		setupErrorPage(request,505);
		goto handleRequestCleanup;
	}

	if (request->client->version.minor > 0 && !findRequestHeader(request, "Host")) {
		setupErrorPage(request, 400);
		goto handleRequestCleanup;
	}

	if(request->requestMethod!=GET && request->requestMethod!=HEAD) {
		setupErrorPage(request,501);
		goto handleRequestCleanup;
	}

	if(!request->requestPath.buff) {
		setupErrorPage(request,400);
		goto handleRequestCleanup;
	}

	if(*request->requestPath.buff!='/') {
		setupErrorPage(request,400);
		goto handleRequestCleanup;
	}

	header=findRequestHeader(request,"Expect");
	if(header && strcmp(header->value,"100-continue") != 0) {
		setupErrorPage(request,417);
		goto handleRequestCleanup;
	}

	badChar=strstr(request->requestPath.buff,"..");
	if(badChar) {
		setupErrorPage(request,403);
		goto handleRequestCleanup;
	}

	badChar=strstr(request->requestPath.buff,"\\");
	if(badChar) {
		setupErrorPage(request,403);
		goto handleRequestCleanup;
	}

	badChar=strstr(request->requestPath.buff,":");
	if(badChar) {
		setupErrorPage(request,403);
		goto handleRequestCleanup;
	}

	if(request->requestPath.buff[request->requestPath.usedLen-2]=='/') {
		request->requestPath.usedLen+=strlen(indexFile)+1;
		if(request->requestPath.usedLen>request->requestPath.allocLen) {
			request->requestPath.allocLen=request->requestPath.usedLen;
			request->requestPath.buff=(char*)realloc(request->requestPath.buff,request->requestPath.allocLen);
		}
		
		strcat(request->requestPath.buff,indexFile);
	}

	fileName=pathToFile(request->requestPath.buff,"pages");

	if(isDeviceFile(fileName)) {
		setupErrorPage(request,403);
		goto handleRequestCleanup;
	}

	if(isLink(fileName)) {
		resolveLink(&fileName);
	}

	replyFile=openNativeFile(fileName,1);

	if(!isValidNativeFile(replyFile)) {
		setupErrorPage(request,404);
		goto handleRequestCleanup;
	}

	request->isFileReply=1;
	request->replyFile=replyFile;
	request->replyCode=200;

	guessMime(request);

	if (request->isFileReply) {
		setLastModifiedHeader(request);
		checkIfModifiedSince(request);
	}


handleRequestCleanup:
	if(fileName) {
		free(fileName);
	}
}

static int checkIfModifiedSince(struct request * request) {
	struct headerListNode *ifmodified;
	time_t fileTime,checkTime;
	struct tm fileTime_tm;


	ifmodified=findRequestHeader(request,"If-Modified-Since");
	if(!ifmodified) return 0;

	checkTime=parseHeaderTimeValue(ifmodified->value);

	fileTime_tm=lastModifiedTime(request->replyFile);

	fileTime=mktime(&fileTime_tm);

	if (fileTime > checkTime) return 0;

	request->replyCode=304;
	return 1;
}

static void convertSlashes(char *path) {
	for(;*path;++path) {
		if(*path=='/') {
			*path='\\';
		}
	}
}

static void setLastModifiedHeader(struct request *request) {

	char buff[35];
	size_t buffLen=31;

	struct tm timeInfo;

	const char *format="%a, %d %b %Y %H:%M:%S GMT";

	timeInfo=lastModifiedTime(request->replyFile);

	if(timeInfo.tm_year==0) return;

	buffLen=strftime(buff,buffLen,format,&timeInfo);

	setReplyHeader(request,"Last-Modified",buff);

}

static void setDateHeader(struct request *request) {
	time_t rawtime;
	struct tm * timeInfo;
	char buff[35];
	size_t buffLen=31;

	const char *format="%a, %d %b %Y %H:%M:%S GMT";

	time( &rawtime );
	timeInfo = gmtime( &rawtime );

	buffLen=strftime(buff,buffLen,format,timeInfo);

	setReplyHeader(request,"Date",buff);
}

static int monthStrToInt(const char *mStr) {
	if(strcmp(mStr,"Jan")==0) return 0;
	if(strcmp(mStr,"Feb")==0) return 1;
	if(strcmp(mStr,"Mar")==0) return 2;
	if(strcmp(mStr,"Apr")==0) return 3;
	if(strcmp(mStr,"May")==0) return 4;
	if(strcmp(mStr,"Jun")==0) return 5;
	if(strcmp(mStr,"Jul")==0) return 6;
	if(strcmp(mStr,"Aug")==0) return 7;
	if(strcmp(mStr,"Sep")==0) return 8;
	if(strcmp(mStr,"Oct")==0) return 9;
	if(strcmp(mStr,"Nov")==0) return 10;
	if(strcmp(mStr,"Dec")==0) return 11;
	return 0;
}

static time_t parseHeaderTimeValue(const char *valStr) {
	struct tm timeVals;

	const char *firstComma;
	char monthBuff[4];

	firstComma=strstr(valStr,",");
	++firstComma;

	sscanf_s(firstComma,"%d %3s %d %d:%d:%d GMT",
		&timeVals.tm_mday,
		monthBuff,4,
		&timeVals.tm_year,
		&timeVals.tm_hour,
		&timeVals.tm_min,
		&timeVals.tm_sec
	);

	timeVals.tm_mon=monthStrToInt(monthBuff);

	timeVals.tm_isdst=0;
	timeVals.tm_yday=0;
	timeVals.tm_wday=0;
	timeVals.tm_year-=1900;

	return mktime(&timeVals);
}

char *pathToFile(char *path,const char *baseDir) {
	struct knownString file;
	
	file.allocLen=strlen(path)+strlen(baseDir)+1;
	file.buff=(char *)malloc(file.allocLen);

	strcpy(file.buff,baseDir);
	strcat(file.buff,path);

#ifdef WIN32
	convertSlashes(file.buff);
#endif

	return file.buff;
}

char * strrstr(char *buff,char *needle) {

	char *lastMatch;

	buff=strstr(buff,needle);
	lastMatch=buff;

	while(buff) {
		buff=strstr(buff+1,needle);
		if(buff) lastMatch=buff;
	}
	
	return lastMatch;
}

static void guessMime(struct request *request) {
	char *extension;
	char *fileName;

	fileName=strrstr(request->requestPath.buff,"/");

	if(!fileName) return;

	extension=strstr(fileName,".");

	if(!extension) return;

	if(strcmp(extension,".html")==0) {
		setReplyHeader(request,"Content-Type","text/html");
	} else if(strcmp(extension,".txt")==0) {
		setReplyHeader(request,"Content-Type","text/plain");
	} else if(strcmp(extension,".css")==0) {
		setReplyHeader(request,"Content-Type","text/css");
	} else if(strcmp(extension,".png")==0) {
		setReplyHeader(request,"Content-Type","image/png");
	} else if(strcmp(extension,".gif")==0) {
		setReplyHeader(request,"Content-Type","image/gif");
	} else if(strcmp(extension,".jpeg")==0 || strcmp(extension,".jpg")==0) {
		setReplyHeader(request,"Content-Type","image/jpeg");
	} else if(strcmp(extension,".ico")==0) {
		setReplyHeader(request,"Content-Type","image/vnd.microsoft.icon");
	} else if(strcmp(extension,".js")==0) {
		setReplyHeader(request,"Content-Type","application/javascript");
	} else if(strcmp(extension,".mp3")==0) {
		setReplyHeader(request,"Content-Type","audio/mpeg");
	} else if(strcmp(extension,".zip")==0) {
		setReplyHeader(request,"Content-Type","application/zip");
	} else {
		setReplyHeader(request,"Content-Type","application/octet-stream");
	}
}

