#pragma once
#include "atto.h"
#include <arpa/inet.h>  // Address Structs
#include <netinet/in.h> // IP Datatypes
#include <sys/socket.h> // Socket Functions

namespace {
class Networking {
  private:
    const uint16_t portNumber;
    int ssock; // Server Socket

    Networking(): portNumber(0), ssock(-1) { }
    Networking(const Networking&): portNumber(0) { }

    int ContructTCPSocket() {
      int O = REUSE_PORT;                        // reuse port sockopt
      struct sockaddr_in saddr;                  // Server Socket addr
      bzero(&saddr, sizeof(saddr));              // Zero serve  struct
      saddr.sin_family = AF_INET;                // Inet addr family
      saddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming iface
      saddr.sin_port = htons(portNumber);        // Local (server) port
      CHECK(ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP), "socket()");
      CHECK(setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &O, sizeof(O)), "opt");
      CHECK(bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)), "bind()");
      CHECK(listen(ssock, MAXPENDING), "listen()");
      return ssock;
    }
    int ServerSocket() { return ssock >= 0 ? ssock : ContructTCPSocket(); }

  public:
    Networking(uint16_t portNumber): portNumber(portNumber), ssock(-1) { }
    int AcceptConnection() {
      struct sockaddr_in addr;
      unsigned len = sizeof(addr);
      int clientSocket = accept(ServerSocket(), (struct sockaddr *)&addr, &len);
      printf("Handling client %s\n", inet_ntoa(addr.sin_addr));
      return (clientSocket < 0) ? -1 : clientSocket;
    }
};
} // end anonymous namespace
