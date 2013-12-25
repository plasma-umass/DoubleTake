#include<stdio.h>
#include <iostream>
using namespace std;
char buf1[4];
char buf2[4];

pthread_mutex_t mutex;
void * thread1 (void *) {
//  sleep(1);
//  if(buf2[0] == '0')
//   pthread_mutex_lock(&mutex);
    buf1[0] = buf2[0] + 1;
//   pthread_mutex_unlock(&mutex);
//  sleep(1);
  return NULL;
}


void * thread2 (void *) {
//  sleep(1);
//  if(buf1[0] == '0')
//   pthread_mutex_lock(&mutex);
    buf2[0] = buf1[0] + 1;
//   pthread_mutex_unlock(&mutex);
//  sleep(1);
  return NULL;
}


main()
{
  buf1[0] = '0';
  buf2[0] = '0';

  pthread_t t1, t2;

  fprintf(stderr, "buf1 %p buf2 %p. buf1[0] %c buf2[0] %c\n", buf1, buf2, buf1[0], buf2[0]);
  pthread_create (&t1, NULL, thread1, NULL);
  pthread_create (&t2, NULL, thread2, NULL);
  pthread_join (t1, NULL);
  pthread_join (t2, NULL);
  cout << buf1[0] << endl;
  cout << buf2[0] << endl;
}
