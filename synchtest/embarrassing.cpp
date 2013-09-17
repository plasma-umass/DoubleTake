#include <pthread.h>

enum { N = 8 };

class Lock {
public:

  Lock() {
    pthread_mutex_init (&_realLock, NULL);
  }

  void lock() {
    pthread_mutex_lock (&_realLock);
  }

  void unlock() {
    pthread_mutex_unlock (&_realLock);
  }

private:
  pthread_mutex_t _realLock;
  double _dummy[64]; // cache padding to avoid false sharing
};

Lock locks[N];
pthread_t threads[N];

void * worker (void * ind) {
  int index = *((int *) ind);
  volatile double d;
  locks[index].lock();
  for (int j = 0; j < 100; j++) {
    for (int i = 0; i < 1000000; i++) {
      d = d * d + d * d;
    }
  }
  locks[index].unlock();
  return NULL;
}


int
main()
{
  for (int i = 0; i < N; i++) {
    int * n = new int (i);
    pthread_create (&threads[i], NULL, worker, (void *) n);
  }
  for (int i = 0; i < N; i++) {
    pthread_join (threads[i], NULL);
  }
  return 0;
}
