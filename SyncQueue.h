#pragma once
#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>

namespace {

#define THREAD_FUNCTION(NAME) template <typename T, typename P, typename C> \
  static void NAME(SyncQueue<T, P, C> *syncQueue)

template <class T, class TP, class TC> class SyncQueue {
  private:
    void(*P)(SyncQueue<T, TP, TC>*);
    void(*C)(SyncQueue<T, TP, TC>*);
    TP ptoken;
    TC ctoken;
    const size_t maxThreads;

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::thread> consumers;
    std::thread producer;
    std::queue<T> syncQueue;
    SyncQueue() { }
    SyncQueue(const SyncQueue&) { }

    void resizeConsumers() {
      for (int n = syncQueue.size() - consumers.size();
           (0 < n) && (consumers.size() < maxThreads); n--)
        consumers.push_back(std::move(std::thread(C, this)));
    }

  public:
    SyncQueue(void(*P)(SyncQueue<T, TP, TC>*), void(*C)(SyncQueue<T, TP, TC>*),
              TP ptoken = nullptr, TC ctoken = nullptr, size_t n = 256):
      P(P), C(C), ptoken(ptoken), ctoken(ctoken), maxThreads(n) { }

    void start() { producer = std::thread(P, this); }
    TP getProducerToken() const { return ptoken; }
    TC getConsumerToken() const { return ctoken; }

    void enqueue(T t) {
      std::unique_lock<std::mutex> lock(mutex);
      syncQueue.push(t);
      resizeConsumers();
      cv.notify_all();
    }

    T dequeue() {
      std::unique_lock<std::mutex> lock(mutex);
      while (!syncQueue.size()) cv.wait(lock);
      T t = syncQueue.front();
      syncQueue.pop();
      return t;
    }
};
} // end anonymous namespace
