#include "atto.h"
#include "SyncQueue.h"

THREAD_FUNCTION(producer) {
  printf("[PRODUCER] Listening on port %d.\n", syncQueue->getProducerToken());
  for (int serverSocket = ContructTCPSocket(syncQueue->getProducerToken());;) {
    printf("[PRODUCER] Waiting to accept a connection\n");
    syncQueue->enqueue(AcceptConnection(serverSocket));
  }
}

THREAD_FUNCTION(consumer) {
  for (printf("[CONSUMER] Consumer started\n");;) {
    int socket = syncQueue->dequeue();
    printf("[CONSUMER] Handling Socket %d.\n", socket);
    if (socket < 0) continue;
    HttpProto(socket,  ReceiveFromSocket(socket).c_str());
    close(socket);
  }
}

int main(int argc, char **argv) {
  std::string LOAD_DIR = ".";
  printf("* Beginning Server, Root Directory: %s\n", LOAD_DIR.c_str());
  CHECK(chdir(LOAD_DIR.c_str()), "Error setting home directory.\n");
  SyncQueue<int, uint16_t, void*> sQueue(producer, consumer, 1337, nullptr);
  sQueue.start();
  while(getchar() != 'q') printf("* Server Started, Enter q to Quit *\n\n");
  return EXIT_SUCCESS;
}

