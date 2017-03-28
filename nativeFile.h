#ifndef NATIVE_FILE_H
#define NATIVE_FILE_H

#include <time.h>

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

typedef HANDLE file_t;
#else
typedef int file_t;
#endif

file_t openNativeFile(char *fileName,int read);
void closeNativeFile(file_t);
int isValidNativeFile(file_t file);
int sizeOfNativeFile(file_t file);

int isDeviceFile(char *fileName);

int isLink(char *fileName);
void resolveLink(char **fileName);

struct tm lastModifiedTime(file_t file);

#endif