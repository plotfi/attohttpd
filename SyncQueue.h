#ifndef _SYNCQUEUE_H_
#define _SYNCQUEUE_H_

#include "atto.h"

template <class T> class SyncQueue {

  private:

    pthread_mutex_t mutex;
    pthread_cond_t isReady;
    std::vector<pthread_t *> consumerThreads;
    pthread_t *producerThread;
    std::queue<T> syncQueue;
    void *token;
    bool isInitialized;
    SyncQueue() {}

  public:

    SyncQueue(void *token, int threads, void *(*producer)(void *),
                                        void *(*consumer)(void *)):
      token(token) {
      printf("** Initializing Thread Pool: %d threads.\n", threads);

      pthread_mutex_init(&mutex, NULL);
      pthread_cond_init(&isReady, NULL);

      consumerThreads.resize(threads);
      producerThread = CheckedMalloc<pthread_t>(sizeof(pthread_t));
      if (pthread_create(producerThread, NULL, producer, (void*)this))
        printf("Error creating producer thread.\n");

      for (unsigned i = 0; i < consumerThreads.size(); ++i) {
        consumerThreads[i] = CheckedMalloc<pthread_t>(sizeof(pthread_t));
        if (pthread_create(consumerThreads[i], NULL, consumer, (void*)this))
          printf("Error creating consumer thread %d.\n", i);
      }
    }

    ~SyncQueue() {
    }

    void *getToken() const { return token; }

    void enqueue(T t) {
      pthread_mutex_lock(&mutex);

      syncQueue.push(t);
      pthread_mutex_unlock(&mutex);
      pthread_cond_signal(&isReady);
    }

    T dequeue() {
      pthread_mutex_lock(&mutex);

      while (syncQueue.size() == 0)
        pthread_cond_wait(&isReady, &mutex); // No requests waiting

      // Fine grain lock, release lock before freeing old node.
      T t = syncQueue.front();
      syncQueue.pop();
      pthread_mutex_unlock(&mutex);
      return t;
    }
};

#endif /* _SYNCQUEUE_H_ */

