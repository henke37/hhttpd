#include <stdio.h>
#include "socket.h"

void listenForConnections(unsigned short int);

int main() {

	initSockets();

	listenForConnections(5555);

	deinitSockets();

	return 0;

}

