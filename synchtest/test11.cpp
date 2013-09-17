
#include <iostream>
using namespace std;
int buf1;
int buf2;

pthread_mutex_t mutex;
void * thread1 (void *) {
//  sleep(1);
//  if(buf2[0] == '0')
//   pthread_mutex_lock(&mutex);
	fprintf(stderr, "thread1 before: buf1 %d buf2 %d\n", buf1, buf2); 
    buf1 = buf2 + 1;
	fprintf(stderr, "thread1: buf1 %d buf2 %d\n", buf1, buf2); 
//   pthread_mutex_unlock(&mutex);
//  sleep(1);
  return NULL;
}


void * thread2 (void *) {
//  sleep(1);
//  if(buf1[0] == '0')
//   pthread_mutex_lock(&mutex);
	fprintf(stderr, "thread2 before: buf1 %d buf2 %d\n", buf1, buf2); 
    buf2 = buf1 + 1;
	fprintf(stderr, "thread2: buf1 %d buf2 %d\n", buf1, buf2); 
//   pthread_mutex_unlock(&mutex);
//  sleep(1);
  return NULL;
}


main()
{
  buf1 = 0;
  buf2 = 0;

  fprintf(stderr, "Buf1 %p buf2 %p\n", &buf1, &buf2);
  pthread_t t1, t2;

  pthread_create (&t1, NULL, thread1, NULL);
  pthread_create (&t2, NULL, thread2, NULL);
  pthread_join (t1, NULL);
  pthread_join (t2, NULL);
  cout << buf1 << endl;
  cout << buf2 << endl;
}
