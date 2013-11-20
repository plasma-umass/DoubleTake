#include <unistd.h>
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
index_t thread_count = 3;
index_t work_count = 3;

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
#if 0
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
#endif
  typedef queue<int> intqueue;
  intqueue* numList = new intqueue();
//  pthread_cond_init(&emptyQueueCond, NULL);
//  pthread_mutex_init(&queueLock, NULL);
  fprintf(stderr, "%d: before calling the pthread_create, numList is %p\n", getpid(), numList);
  printf("Finish the main\n");
}

