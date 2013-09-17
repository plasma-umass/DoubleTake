#include <queue>
#include <sstream>
#include <string>
#include <fstream>
#include <cassert>
//#include <iostream>
#include <pthread.h>
#include <errno.h>
#include <vector>
#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>

using namespace std;
typedef size_t index_t;

//index_t thread_count = 4;
//index_t work_count = 10;
index_t thread_count = 2;
index_t work_count = 2;

pthread_mutex_t queueLock;
pthread_cond_t  emptyQueueCond;
bool done;

void* getLocation(void *in_siteList)
{
  queue<int> *siteList;
  siteList = static_cast<queue<int>*> (in_siteList);
  while (true)
  {
    pthread_mutex_lock(&queueLock);
    while (siteList->empty() )
    {
	  // Here, queue is empty
      if (done)
      {
		// If we recieve the "done" message, send a signal.
        pthread_cond_signal(&emptyQueueCond);
        pthread_mutex_unlock(&queueLock);
        return NULL;
      }
	    //Otherwise, we are going to wait on the queue.
	    fprintf(stderr, "%d Waiting...\n", getpid());
      pthread_cond_wait(&emptyQueueCond, &queueLock);
   }

   // Get one work from the queue.
   int curNum = siteList->front();
   siteList->pop();
   fprintf(stderr, "%d: curNum %d!!!!!\n", getpid(), curNum);
   // Help to wake up other threads.
   pthread_cond_signal(&emptyQueueCond);
   pthread_mutex_unlock(&queueLock);
   }
}



int main(int argc, char* argv[])
{
  if (argc == 2)
  {
    stringstream s1;
    s1 << argv[1];
    s1 >> thread_count;
  }
  done = false;
  vector <pthread_t> threadList;
  threadList.resize(thread_count);
  void* exit_status;
  typedef queue<int> intqueue;
  intqueue* numList = new intqueue();
  pthread_cond_init(&emptyQueueCond, NULL);
  pthread_mutex_init(&queueLock, NULL);
  fprintf(stderr, "%d: before calling the pthread_create, numList is %p\n", getpid(), numList);
  for (int i = 0; i < thread_count; i++)
  {
    pthread_create(&threadList[i], NULL, getLocation, numList);
    fprintf(stderr, "Creating thread %d\n", i);
  }

  // Put some work in the queue and wake up at least one thread.
  for (int i = 0; i < work_count; i++)
  {
    pthread_mutex_lock(&queueLock);
    numList->push(i);
	  fprintf(stderr, "%d: wakingup %d\n", getpid(), i);
    int ret = pthread_cond_signal(&emptyQueueCond);
    while (ret != 0) {
        ret = pthread_cond_signal(&emptyQueueCond); // wake up atleast one thread
    }
    pthread_mutex_unlock(&queueLock);
  }
  fprintf(stderr, "%d: We are done in main thread\n", getpid());

  // Send out the signal.
  pthread_mutex_lock(&queueLock);
  done = true;
  pthread_cond_signal(&emptyQueueCond);
  pthread_mutex_unlock(&queueLock);

  for (int i = 0; i < threadList.size(); i++)
  {
     fprintf(stderr, "Trying to join: %lx\n", threadList[i]);
     pthread_join(threadList[i], NULL);
  }
  printf("Finish the main\n");
}

