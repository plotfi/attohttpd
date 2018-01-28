#include "atto.h"
#include "SyncQueue.h"

THREAD_FUNCTION(producer) {
  printf("[PRODUCER] Listening on port %d.\n", syncQueue->getProducerToken());
  for (int serverSocket = ContructTCPSocket(syncQueue->getProducerToken());;) {
    printf("[PRODUCER] Waiting to accept a connection\n");
    int clientSocket = AcceptConnection(serverSocket);
    if (clientSocket >= 0) syncQueue->enqueue(clientSocket);
  }
}

THREAD_FUNCTION(consumer) {
  for (printf("[CONSUMER] Consumer started\n");;) {
    int socket = syncQueue->dequeue();
    printf("[CONSUMER] Handling Socket %d.\n", socket);
    HttpProtoWrapper(socket,  ReceiveFromSocket(socket).c_str());
    close(socket);
  }
}

int main(int argc, char **argv) {
  std::string LOAD_DIR = ".";
  printf("* Beginning Server Initialization *\n\n");
  printf("** Server Root Directory: %s\n", LOAD_DIR.c_str());
  CHECK(chdir(LOAD_DIR.c_str()), "Error setting home directory.\n");
  SyncQueue<int, uint16_t, void*> sQueue(producer, consumer, 1337, nullptr);
  sQueue.start();
  printf("* Server Started *\n\n");
  while(getchar() != 'q');
  return EXIT_SUCCESS;
}

