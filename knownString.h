#ifndef KNOWN_STRING_H
#define KNOWN_STRING_H

#include <string.h>

struct knownString {
	char *buff;
	size_t usedLen,allocLen;
};
#endif
