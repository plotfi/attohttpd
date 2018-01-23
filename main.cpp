#include "atto.h"

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

int main(int argc, char **argv) {
  std::string LOAD_DIR = ".";

  printf("* Beginning Server Initialization *\n\n");
  printf("** Server Root Directory: %s\n", LOAD_DIR.c_str());
  if (chdir(LOAD_DIR.c_str()) < 0) {
    printf("Error setting home directory.\n");
    exit(1);
  }

  InitThreads(256 /* THREADS */, 1337 /* PORT */);
  printf("* Server Started *\n\n");

  for (int c = '\0'; c != 'q';)
    c = getchar();

  return EXIT_SUCCESS;
}

