#ifndef __SERVER_H
#define __SERVER_H

#include "remoteGame.h"

int server(int port, RemoteGame &);
int handleChild(int socket, RemoteGame &);

#endif
