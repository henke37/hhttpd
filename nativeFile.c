#include "nativeFile.h"

char * strrstr(char *buff,char *needle);

#ifdef WIN32

file_t openNativeFile(char *fileName,int read) {
	file_t handle;

	if(read) {
		handle=CreateFile(fileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	} else {
		handle=CreateFile(fileName,GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	}

	return handle;
}

int isValidNativeFile(file_t handle) {
	return handle!=INVALID_HANDLE_VALUE;
}

void closeNativeFile(file_t handle) {
	CloseHandle(handle);
}

int sizeOfNativeFile(file_t file) {
	return GetFileSize(file,NULL);
}

int isDeviceFile(char *fileName) {

	char *lastRSlash=strrstr(fileName,"\\");

	if(!lastRSlash) return 0;

	lastRSlash++;

	if(_strcmpi(lastRSlash,"CON")==0) return 1;
	if(_strcmpi(lastRSlash,"PRN")==0) return 1;
	if(_strcmpi(lastRSlash,"AUX")==0) return 1;
	if(_strcmpi(lastRSlash,"NUL")==0) return 1;
	if(_strcmpi(lastRSlash,"CLOCK$")==0) return 1;

	if(_strcmpi(lastRSlash,"LPT1")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT2")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT3")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT4")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT5")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT6")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT7")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT8")==0) return 1;
	if(_strcmpi(lastRSlash,"LPT9")==0) return 1;

	if(_strcmpi(lastRSlash,"COM1")==0) return 1;
	if(_strcmpi(lastRSlash,"COM2")==0) return 1;
	if(_strcmpi(lastRSlash,"COM3")==0) return 1;
	if(_strcmpi(lastRSlash,"COM4")==0) return 1;
	if(_strcmpi(lastRSlash,"COM5")==0) return 1;
	if(_strcmpi(lastRSlash,"COM6")==0) return 1;
	if(_strcmpi(lastRSlash,"COM7")==0) return 1;
	if(_strcmpi(lastRSlash,"COM8")==0) return 1;
	if(_strcmpi(lastRSlash,"COM9")==0) return 1;

	return 0;
}

int isLink(char *fileName) {
	return 0;
}
void resolveLink(char **fileName) {}

struct tm lastModifiedTime(file_t file) {
	FILETIME ftCreate, ftAccess, ftWrite;
  SYSTEMTIME stUTC;
  struct tm tmbuf;

  // Retrieve the file times for the file.
  if (GetFileTime(file, &ftCreate, &ftAccess, &ftWrite)) {

		// Convert the last-write time to local time.
		FileTimeToSystemTime(&ftWrite, &stUTC);

		tmbuf.tm_hour=stUTC.wHour;
		tmbuf.tm_isdst=0;
		tmbuf.tm_mday=stUTC.wDay;
		tmbuf.tm_min=stUTC.wMinute;
		tmbuf.tm_mon=stUTC.wMonth-1;
		tmbuf.tm_sec=stUTC.wSecond;
		tmbuf.tm_year=stUTC.wYear-1900;
		tmbuf.tm_wday=0;
		tmbuf.tm_yday=0;
	} else {
		tmbuf.tm_hour=0;
		tmbuf.tm_mday=0;
		tmbuf.tm_min=0;
		tmbuf.tm_sec=0;
		tmbuf.tm_wday=0;
		tmbuf.tm_yday=0;
		tmbuf.tm_year=0;
	}

	return tmbuf;

}

#else

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

file_t openNativeFile(char *fileName,int forRead) {
	file_t handle;

	if(forRead) {
		handle=open(fileName,O_RDONLY);
	} else {
		handle=open(fileName,O_WRONLY|O_CREAT,0666);
	}

	return handle;
}

int isValidNativeFile(file_t handle) {
	return handle!=-1;
}

void closeNativeFile(file_t handle) {
	close(handle);
}

int sizeOfNativeFile(file_t file) {
	struct stat sta;
	fstat(file,&sta);
	return sta.st_size;
}

int isLink(char *fileName) {
	return 0;
}
void resolveLink(char **fileName) {}

int isDeviceFile(char *fileName) {
	return 0;
}

#endif
