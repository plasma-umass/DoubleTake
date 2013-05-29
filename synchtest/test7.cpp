// Created from the "test/testcondvar.cpp"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_cond_t sent;
pthread_mutex_t sentLock;
pthread_cond_t received;
pthread_mutex_t receivedLock;

int v;
bool sth = false;

int  times = 2;

void * prod (void *) {
  int q = 0;
  for (int i = 0; i < times; i++) {
    pthread_mutex_lock(&sentLock);
    v = q;
    q++;
    sth = true;
	  pthread_cond_signal(&sent);
    fprintf (stderr, "PRODUCE %d\n", v); 
    pthread_mutex_unlock(&sentLock);

	//fflush (stdout);
    pthread_mutex_lock(&receivedLock);
	  fprintf(stderr, "PRODUCE %d sth is %d\n", v, sth);
	  while(sth == true) {
    	pthread_cond_wait (&received, &receivedLock);
	  }
    pthread_mutex_unlock(&receivedLock);
  }
  return 0;
}

// Consumer always set sth to false after consuming it.
void * cons (void *) {
  int q;
  for (int i = 0; i < times; i++) {
    pthread_mutex_lock(&sentLock);
	  while(sth == false) {
   // 	fprintf (stderr, "waiting for producer on %d\n", i); 
    	pthread_cond_wait (&sent, &sentLock);
	  }
    q = v;
    pthread_mutex_unlock(&sentLock);
	// fflush (stdout);

    pthread_mutex_lock(&receivedLock);
	  sth = false;
    fprintf (stderr, "CONSUME %d\n", q);
	  pthread_cond_signal(&received);
    pthread_mutex_unlock(&receivedLock);
  }
  return 0;
}


int main(int argc, char ** argv)
{
  v = 0;
  sth = false;

  pthread_cond_init (&sent, NULL);
  pthread_cond_init (&received, NULL);
  pthread_mutex_init (&sentLock, NULL);
  pthread_mutex_init (&receivedLock, NULL);

  fprintf(stderr, "sth is %d on %p\n", sth, &sth);
  if(argc == 2) {
	  times = atoi(argv[1]);
  }

  pthread_t threads[2];
  pthread_create (&threads[0], NULL, prod, NULL);
  pthread_create (&threads[1], NULL, cons, NULL);

  for (int i = 0; i < 2; i++) {
    pthread_join (threads[i], NULL);
  }
  return 0;
}

