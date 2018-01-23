#include "atto.h"
#include "SyncQueue.h"

void *producer(void *s) {
  SyncQueue<int> *syncQueue = (SyncQueue<int>*)s;

  const uint16_t portNumber = *((uint16_t *)syncQueue->getToken());
  printf("[PRODUCER] Initialized, listening on port %d.\n", portNumber);

  for (int serverSocket = ContructTCPSocket(portNumber);;) {
    int clientSocket = AcceptConnection(serverSocket);
    if (clientSocket >= 0) syncQueue->enqueue(clientSocket);
  }

  assert(false && "Never Reached!");
  return nullptr;
}

void *consumer(void *s) {
  printf("[CONSUMER] Consumer %ld started\n", (long)pthread_self());

  SyncQueue<int> *syncQueue = (SyncQueue<int>*)s;
  char curToRecv[BUFFERSIZE];
  for (;;) {
    bzero(curToRecv, BUFFERSIZE);

    int socket = syncQueue->dequeue();
    printf("[CONSUMER] Handling Socket %d.\n", socket);

    if (char *curToSend = ReceiveFromSocket(socket, curToRecv, BUFFERSIZE))
      HttpProtoWrapper(socket, curToSend);

    close(socket);
  }

  assert(false && "Never Reached!");
  return nullptr;
}

int main(int argc, char **argv) {
  std::string LOAD_DIR = ".";

  printf("* Beginning Server Initialization *\n\n");
  printf("** Server Root Directory: %s\n", LOAD_DIR.c_str());
  if (chdir(LOAD_DIR.c_str()) < 0) {
    printf("Error setting home directory.\n");
    exit(1);
  }

  const uint16_t portNumber = 1337;
  SyncQueue<int>((void*)&portNumber, 256 /* THREADS */, producer, consumer);
  printf("* Server Started *\n\n");

  for (int c = '\0'; c != 'q';)
    c = getchar();

  return EXIT_SUCCESS;
}

