#include "atto.h"

static pthread_mutex_t mutex;
static pthread_cond_t isReady;

static std::queue<int> sockets;
static std::vector<pthread_t *> consumerThreads;
static pthread_t *producerThread;
static pthread_mutex_t *get_mutex() { return &mutex; }
static pthread_cond_t *get_is_ready() { return &isReady; }

void addSocket(int socket) {
  VPRINTF(("Adding socket: %d\n", socket));
  pthread_mutex_lock(get_mutex());

  sockets.push(socket);
  pthread_mutex_unlock(get_mutex());
  pthread_cond_signal(get_is_ready());
}

int removeSocket() {
  pthread_mutex_lock(get_mutex());

  while (sockets.size() == 0)
    pthread_cond_wait(get_is_ready(), get_mutex()); // No requests waiting

  // Fine grain lock, release lock before freeing old node
  int socket = sockets.front();
  VPRINTF(("Removing socket: %d\n", socket));
  sockets.pop();
  pthread_mutex_unlock(get_mutex());

  return socket;
}

void InitThreads(int THREADS, uint16_t PORT) {
  printf("** Initializing Thread Pool\n");
  printf("*** Listening Port: %d, Initializing %d Consumers.\n", PORT, THREADS);

  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&isReady, NULL);

  consumerThreads.resize(THREADS);
  producerThread = CheckedMalloc<pthread_t>(sizeof(pthread_t));
  if (pthread_create(producerThread, NULL, producer, &(PORT)))
    printf("Error creating producer thread.\n");

  for (unsigned i = 0; i < consumerThreads.size(); ++i) {
    consumerThreads[i] = CheckedMalloc<pthread_t>(sizeof(pthread_t));
    if (pthread_create(consumerThreads[i], NULL, consumer, NULL))
      printf("Error creating consumer thread %d.\n", i);
  }
}

