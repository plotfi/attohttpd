#include <arpa/inet.h>  /* Address Structs  */
#include <netinet/in.h> /* IP Datatypes     */
#include <sys/socket.h> /* Socket Functions */
#include <sys/types.h>  /* Socket Datatypes */

#include "atto.h"

#define DieWithError(check, message) \
  do { \
    if ((check) < 0) { \
      fprintf(stderr, "%s failed: Error on line %d.\n", (message), __LINE__); \
      perror(message); \
      exit(EXIT_FAILURE); \
    } \
  } while (false)

void *producer(void *v_port_number) {

  const uint16_t port_number = *((uint16_t *)v_port_number);

  VPRINTF((" Producer Port %d\n", port_number));
  printf("[PRODUCER] Producer Initialized, listening for connections\n");
  fflush(stdout);

  int serverSocket = ContructTCPSocket(port_number);
  for (;;) {
    int clientSocket = AcceptConnection(serverSocket);
    if (clientSocket == -1) continue;
    addSocket(clientSocket);
  }

  assert(false && "Never Reached!");
}

int ContructTCPSocket(uint16_t portNumber) {
  struct sockaddr_in saddr;                  // Server Socket addr
  bzero(&saddr, sizeof(saddr));           // Zero serve  struct
  saddr.sin_family = AF_INET;                // Inet addr family
  saddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming iface
  saddr.sin_port = htons(portNumber);        // Local (server) port

  int ssock; // Server Socket
  int opt = REUSE_PORT; // reuse port sockopt
  DieWithError(ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP), "socket()");
  DieWithError(setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)),
               "setsockopt()");
  DieWithError(bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)), "bind()");
  DieWithError(listen(ssock, MAXPENDING), "listen()");

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
  char *curToSend;
  int clntSocket;
  char curToRecv[BUFFERSIZE];
  char *curToRecv_ptr;
  char *dgramPtr;
  int totalBytesRecv;
  int newBytesRecv;
  int i;

  #if _VERBOSE_
  printf("[CONSUMER] Consumer %ld started\n", (long)pthread_self());
  fflush(stdout);
  #endif

  for (;;) {

    clntSocket = removeSocket(); // This is where it waits!!!

    #if _VERBOSE_
    printf("[CONSUMER] Handling Socket %d\n", clntSocket);
    fflush(stdout);
    #endif

    memset(curToRecv, 0, sizeof(char) * BUFFERSIZE);

    for (i = 0, totalBytesRecv = 0, newBytesRecv = 0, dgramPtr = curToRecv;
         totalBytesRecv < sizeof(curToRecv);
         totalBytesRecv += newBytesRecv, i++) {

      VPRINTF(("Receiving %d Bytes. %d Bytes Total.\n", newBytesRecv,
               totalBytesRecv));

      if (((newBytesRecv = recv(clntSocket, dgramPtr,
                                sizeof(curToRecv) - totalBytesRecv, 0)) <= 0)) {
        VPRINTF(("recv() failed\n"));
        VPRINTF(("Kill connection line %d.\n", __LINE__));
        close(clntSocket);
        // pthread_exit(NULL);
      } else {
        dgramPtr += newBytesRecv;

        VPRINTF(("%s\n", curToRecv));

        if (NULL != (curToSend = strtok_r(curToRecv, "\n", &curToRecv_ptr))) {
          if (curToRecv[strlen(curToSend) - 1] == '\r') {
            VPRINTF(("GOT A CR\n"));

            break; /* HACK */
            if (curToRecv[strlen(curToSend) - 3] == '\r' &&
                curToRecv[strlen(curToSend) - 2] == '\n') {
              break;
            } else {
              curToRecv[strlen(curToSend)] = '\n';
              continue;
            }
          } else {
            curToRecv[strlen(curToSend)] = '\n';
            continue;
          }
        }
      }
    }

    VPRINTF(("Out of the Recv() Loop\n"));
    VPRINTF(("clntSocket: %d\n", clntSocket));
    fflush(stdout);

    if (!curToSend) {
      VPRINTF(("curToSend == NULL :(\n"));
      continue;
    }

    HttpProtoWrapper(clntSocket, curToSend);
    VPRINTF(("Socket %d says goodbye.\n", (int)clntSocket));
    fflush(stdout);
  }

  assert(false && "Never Reached!");
  return NULL;
}

