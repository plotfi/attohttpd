#ifndef _ATTO_H_
#define _ATTO_H_

#define REUSE_PORT 1
#define MAXPENDING 5  // Max Requests
#define BUFFERLEN 100 // recv size per iter
#define HTTPD "attohttpd"
#define URL "http://www.puyan.org"

#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>

int HttpProto(int socket);

#define CHECK(check, message) \
  do { if ((check) < 0) { \
      fprintf(stderr, "%s failed: Error on line %d.\n", (message), __LINE__); \
      perror(message); exit(EXIT_FAILURE); \
  } } while (false)

#endif /* _ATTO_H_ */
