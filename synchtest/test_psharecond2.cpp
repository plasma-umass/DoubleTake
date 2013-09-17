/* This test file will verify whether pthread_cond_signal mechanism can work on multi-process environement. */
#include <stdio.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define PAGE_SIZE 4096

struct testStruct {
	int bModifiedBy; /* 0 - if parent modify. 1 if child modify that. */
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};
	 
int main(int argc, char **argv)
{
	struct testStruct * pShare;	
	int child;
	pthread_mutexattr_t mutexAttr;
	pthread_condattr_t condAttr;

    typedef int (*condSignalFunc) (pthread_cond_t *);
    static condSignalFunc realCondSignal;

    typedef int (*condWaitFunc) (pthread_cond_t *, pthread_mutex_t *);
    static condWaitFunc realCondWait;

    realCondWait = (condWaitFunc)dlsym (0, "pthread_cond_wait");
    realCondSignal = (condSignalFunc)dlsym (0, "pthread_cond_signal");

	// Mmap an MAP_SHARED area to store cond and mutex.
	pShare = (struct testStruct *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	// Do initialization.
    pShare->bModifiedBy = 0;
	pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared (&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&pShare->mutex, &mutexAttr);
	
	pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared (&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&pShare->cond, &condAttr);
    
    // Then try to create the child and do some experiments on that 
    child = fork();
	if(child == -1) {
		perror("Fail to create a child\n");
		_exit(-1);
	}

	if(child == 0) {
		// I am the child. 
		
		printf("Child %d will sleep for 50 us\n", getpid());

		// First, take a rest.
	    usleep(50);
		
		// Modify the share variable.
		pShare->bModifiedBy = 1;

	    printf("Child will send out signal.\n");
		// Then notify the parent using the cond_signal
		realCondSignal(&pShare->cond);

		// Be sure that parent will receive the signal, I will wait for the parent's notification too.
		pthread_mutex_lock(&pShare->mutex);
		
		while(pShare->bModifiedBy != 0) { 
			//printf("Child will wait for the parent to notify me. The condwait address %x\n", (int)realCondWait);
			printf("Child will wait for the parent to notify me\n");

			realCondWait(&pShare->cond, &pShare->mutex);
			printf("Child are waken up...\n");
		}
		
		pthread_mutex_unlock(&pShare->mutex);
		printf("Child %d have receive the notification from parent and try to exit now.\n", getpid());
		// Since parent have received the signal and notify me, then I can exit successfully.
		_exit(0);
		
	} else {
		// I am the parent.
		int status;

		// wait for the child to notify me using the conditional variable.  
		pthread_mutex_lock(&pShare->mutex);
		while(pShare->bModifiedBy != 1) { 
			printf("Parent will wait for the child to notify me.\n");
			realCondWait(&pShare->cond, &pShare->mutex);
			printf("Parent are waken up...\n");
		}
		pthread_mutex_unlock(&pShare->mutex);
		printf("parent recieve the signal from child.\n");

		// Also, we will notify the child too.
		pShare->bModifiedBy = 0;
	    printf("Parent will send out signal.\n");
		realCondSignal(&pShare->cond);
	
		// Then I will wait for the child to exit.
		wait(&status);
		printf("parent find out child has exited\n");
	}

}
