#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

/*
    This program performs the following sequence of events:

    thread 1            thread 2
     (main)           (worker_main)

    pthread_create                
    malloc(array)                 

                      write(array)
                      free(array)

    write(array)

                      pthread_exit

    pthread_join
    exit

    This order-of-events is enforced through atomic reads + writes of
    a global sequence number, avoiding unspecified behavior as well as
    the use of mutexes, so that we don't trigger any intermediate
    epoch expirations.

    This is a classic use-after-free, and is properly identified as
    such by valgrind:

    ==15506== Invalid write of size 1
    ==15506==    at 0x4008B5: main (main.c:122)
    ==15506==  Address 0x53ed040 is 0 bytes inside a block of size 255 free'd
    ==15506==    at 0x4C2B1F0: free (in /usr/lib64/valgrind/vgpreload_memcheck-amd64-linux.so)
    ==15506==    by 0x400794: worker_main (main.c:86)
    ==15506==    by 0x4E3D333: start_thread (pthread_create.c:333)

    As well as tsan:

    ==================
    WARNING: ThreadSanitizer: heap-use-after-free (pid=16214)
      Write of size 1 at 0x7d4000007f00 by main thread:
        #0 main /home/bpowers/plasma/DoubleTake/test/simple_mt_uaf/main.c:122:2 (simple.test+0x0000004b86a3)

        n  Previous write of size 8 at 0x7d4000007f00 by thread T1:
        #0 free /var/tmp/portage/sys-devel/llvm-3.6.2/work/llvm-3.6.2.src/projects/compiler-rt/lib/tsan/rtl/tsan_interceptors.cc:538:3 (simple.test+0x00000045267b)
        #1 worker_main /home/bpowers/plasma/DoubleTake/test/simple_mt_uaf/main.c:86:2 (simple.test+0x0000004b846c)

      Thread T1 (tid=16216, running) created by main thread at:
        #0 pthread_create /var/tmp/portage/sys-devel/llvm-3.6.2/work/llvm-3.6.2.src/projects/compiler-rt/lib/tsan/rtl/tsan_interceptors.cc:896:3 (simple.test+0x000000455f51)
        #1 main /home/bpowers/plasma/DoubleTake/test/simple_mt_uaf/main.c:107:2 (simple.test+0x0000004b85bb)

    SUMMARY: ThreadSanitizer: heap-use-after-free /home/bpowers/plasma/DoubleTake/test/simple_mt_uaf/main.c:122 main
    ==================
    ThreadSanitizer: reported 1 warnings
*/

#define ARRAY_SIZE   0xff

// sequence number - use this to coordinate memory operation orders
// without mutexes (so that coordination between threads doesn't
// trigger epoch expirations)
int       g_seq;
uintptr_t g_array;

// release the cacheline on the variable we're waiting for
static inline void spin() {
	__asm__ __volatile__("pause" : : : "memory");
}

void *
worker_main(void *arg) {
	char n = (char)(uintptr_t)arg;

	// wait for the main thread to store the dynamically allocated
	// array in g_array
	while (__atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) != 1)
		spin();

	// get the location of the array, write to it, and free it.
	char *array = (char *)__atomic_load_n(&g_array, __ATOMIC_SEQ_CST);
	for (size_t i = 0; i < ARRAY_SIZE; i++)
		array[i] = n;
	free(array);

	// inform the main thread that we have free'd the array
	__atomic_store_n(&g_seq, 2, __ATOMIC_RELEASE);

	// wait for main thread to perform a use-after-free before we exit
	while (__atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) != 3)
		spin();

	//pthread_exit(NULL);
	return NULL;
}

int
main(int argc, const char *argv[]) {
	pthread_t worker;

	char *array = calloc(ARRAY_SIZE, sizeof(*array));
	if (!array)
		return -1;

	pthread_create(&worker, NULL, worker_main, (void *)0x11);

	// pass the array to the worker thread, and increase the
	// sequence number, allowing the worker to progress into the
	// 'write + free' phase.
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	__atomic_store_n(&g_array, (uintptr_t)array, __ATOMIC_SEQ_CST);
	__atomic_store_n(&g_seq, 1, __ATOMIC_SEQ_CST);

	// wait for the worker to free the array
	while (__atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) != 2)
		spin();

	// at this point, the array has been free'd in the worker
	// thread - perform a use-after-free
	array[0] = 0;

	// tell the worker it can exit
	__atomic_store_n(&g_seq, 3, __ATOMIC_SEQ_CST);

	// wait for the worker to exit
	pthread_join(worker, NULL);

	return 0;
}
