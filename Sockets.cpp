#include <arpa/inet.h>  /* Address Structs  */
#include <netinet/in.h> /* IP Datatypes     */
#include <sys/socket.h> /* Socket Functions */
#include <sys/types.h>  /* Socket Datatypes */
#include "atto.h"

int ContructTCPSocket(uint16_t portNumber) {
  struct sockaddr_in saddr;                  // Server Socket addr
  bzero(&saddr, sizeof(saddr));              // Zero serve  struct
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
  struct sockaddr_in addr;
  unsigned len = sizeof(addr);
  int clientSocket = accept(serverSocket, (struct sockaddr *)&addr, &len);
  printf("Handling client %s\n", inet_ntoa(addr.sin_addr));
  return (clientSocket < 0) ? -1 : clientSocket;
}

std::string ReceiveFromSocket(int socket) {
  char buffer[BUFFERLEN];
  std::string resultStr("");
  for (;;) {
    bzero(buffer, BUFFERLEN);
    int newBytes = recv(socket, buffer, BUFFERLEN, 0);
    resultStr += (newBytes > 0) ? std::string(buffer) : "";
    if (newBytes < BUFFERLEN) break;
  }
  return resultStr;
}

