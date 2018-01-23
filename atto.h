#ifndef _UHTTP_H_
#define _UHTTP_H_

#define _VERBOSE_ 0
#define REUSE_PORT 1
#define MAXPENDING 5    // Max Requests
#define BUFFERSIZE 1024 // Max Recv Buffer Size
#define SERVER_NAME "attohttpd"
#define SERVER_URL "http://www.puyan.org"
#define VPRINTF(x) if (_VERBOSE_) { printf x; }

#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>

#include <queue>
#include <vector>
#include <string>

int ContructTCPSocket(uint16_t port);
int AcceptConnection(int socket);
char *ReceiveFromSocket(int socket, char *buffer, size_t len);
int HttpProtoWrapper(int socket, char *request);

template <typename T> T *CheckedMalloc(size_t n) {
  T *result = (T*)malloc(n);
  if (!result) {
    printf("Malloc failed. Exiting.\n");
    exit(EXIT_FAILURE);
  }
  return result;
}

#endif /* _UHTTP_H_ */

