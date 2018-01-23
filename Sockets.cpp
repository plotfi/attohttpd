#include <arpa/inet.h>  /* Address Structs  */
#include <netinet/in.h> /* IP Datatypes     */
#include <sys/socket.h> /* Socket Functions */
#include <sys/types.h>  /* Socket Datatypes */

#include "atto.h"

#define CHECK(check, message) \
  do { \
    if ((check) < 0) { \
      fprintf(stderr, "%s failed: Error on line %d.\n", (message), __LINE__); \
      perror(message); \
      exit(EXIT_FAILURE); \
    } \
  } while (false)

int ContructTCPSocket(uint16_t portNumber) {
  struct sockaddr_in saddr;                  // Server Socket addr
  bzero(&saddr, sizeof(saddr));           // Zero serve  struct
  saddr.sin_family = AF_INET;                // Inet addr family
  saddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming iface
  saddr.sin_port = htons(portNumber);        // Local (server) port

  int ssock; // Server Socket
  int opt = REUSE_PORT; // reuse port sockopt
  CHECK(ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP), "socket()");
  CHECK(setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), "sopt");
  CHECK(bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)), "bind()");
  CHECK(listen(ssock, MAXPENDING), "listen()");
  return ssock;
}

int AcceptConnection(int serverSocket) {
  VPRINTF(("[PRODUCER] Waiting to accept a connection\n"));
  struct sockaddr_in addr;
  unsigned len = sizeof(addr);
  int clientSocket = accept(serverSocket, (struct sockaddr *)&addr, &len);
  VPRINTF(("Handling client %s\n", inet_ntoa(addr.sin_addr)));

  // If negative return -1 so it knows to ignore this connection.
  return (clientSocket < 0) ? -1 : clientSocket;
}

char *ReceiveFromSocket(int socket, char *buffer, size_t len) {
  char *curToSend = nullptr;

  for (int bytesRecv = 0; bytesRecv < len;) {
    int newBytes = recv(socket, buffer + bytesRecv, len - bytesRecv, 0);
    bytesRecv += newBytes;
    if (newBytes <= 0) return nullptr; // recv failed, start over.

    char *ptr;
    if ((curToSend = strtok_r(buffer, "\n", &ptr))) {
      if (buffer[strlen(curToSend) - 1] == '\r')
        return curToSend;
      buffer[strlen(curToSend)] = '\n';
    }
  }

  return curToSend;
}

int http_proto(FILE *socket, char *request);
int HttpProtoWrapper(int socket, char *request) {
  FILE *socketFile = fdopen(socket, "w");
  fflush(socketFile);
  int result = http_proto(socketFile, request);
  fflush(socketFile);
  fclose(socketFile);
  return result;
}

void *consumer(void *dumb) {
  printf("[CONSUMER] Consumer %ld started\n", (long)pthread_self());

  for (;;) {
    char curToRecv[BUFFERSIZE];
    bzero(curToRecv, BUFFERSIZE);

    int socket = removeSocket(); // Blocking IO: This is where it waits!!!
    printf("[CONSUMER] Handling Socket %d\n", socket);

    if (char *curToSend = ReceiveFromSocket(socket, curToRecv, BUFFERSIZE))
      HttpProtoWrapper(socket, curToSend);

    close(socket);
  }

  assert(false && "Never Reached!");
  return nullptr;
}

