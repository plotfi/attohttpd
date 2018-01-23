#ifndef _UHTTP_H_
#define _UHTTP_H_

#define SERVER_NAME "attohttpd"
#define SERVER_URL "http://www.puyan.org"

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

// Logging Macros
#define _VERBOSE_ 0
#define VPRINTF(x)                                                             \
  if (_VERBOSE_)                                                               \
    printf x;

// Networking Headers
void *consumer(void *);
void *producer(void *v_port_number);
int ContructTCPSocket(unsigned short port);
int AcceptConnection(int server_socket);
#define MAXPENDING 5    // Max Requests
#define BUFFERSIZE 1024 // Max Recv Buffer Size
#define REUSE_PORT 1

void addSocket(int);
int removeSocket();
void InitThreads(int THREADS, uint16_t PORT);

template <typename T> T *CheckedMalloc(size_t n) {
  T *result = (T*)malloc(n);
  if (!result) {
    printf("Malloc failed. Exiting.\n");
    exit(EXIT_FAILURE);
  }
  return result;
}

#endif /* _UHTTP_H_ */

