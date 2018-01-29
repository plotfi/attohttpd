#include "atto.h"
#include "SyncQueue.h"
#include "Networking.h"

THREAD_FUNCTION(producer) {
  for (printf("[PRODUCER] Listening\n");;) {
    printf("[PRODUCER] Waiting to accept a connection\n");
    syncQueue->enqueue(syncQueue->getProducerToken()->AcceptConnection());
  }
}

THREAD_FUNCTION(consumer) {
  for (int S = -1;;S = syncQueue->dequeue()) {
    printf("[CONSUMER] Handling Socket %d.\n", S);
    if (S >= 0) close(HttpProto(S));
  }
}

int main(int argc, char **argv) {
  std::string LOAD_DIR = ".";
  printf("* Beginning Server, Root Directory: %s\n", LOAD_DIR.c_str());
  CHECK(chdir(LOAD_DIR.c_str()), "Error setting home directory.\n");
  Networking N(1337);
  SyncQueue<int, Networking*, void*> sQueue(producer, consumer, &N, nullptr);
  sQueue.start();
  while(getchar() != 'q') printf("* Server Started, Enter q to Quit *\n\n");
  return EXIT_SUCCESS;
}
