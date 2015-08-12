/**
 * @file libdoubletake.cpp
 * @brief Interface with outside library.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 */

#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "globalinfo.hh"
#include "real.hh"
#include "syscalls.hh"
#include "xmemory.hh"
#include "xrun.hh"
#include "xthread.hh"

// Install xxmalloc, xxfree, etc. as custom allocator
#include "wrappers/gnuwrapper.cpp"

using namespace std;
enum { InitialMallocSize = 1024 * 1024 * 1024 };

size_t __max_stack_size;

bool funcInitialized = false;
bool initialized = false;

// Some global information.
bool g_isRollback;
bool g_hasRollbacked;
int g_numOfEnds;
enum SystemPhase g_phase;
pthread_cond_t g_condCommitter;
pthread_cond_t g_condWaiters;
pthread_mutex_t g_mutex;
pthread_mutex_t g_mutexSignalhandler;
int g_waiters;
int g_waitersTotal;
#ifdef GET_CHARECTERISTICS
unsigned long count_epochs = 0;
#endif
void initRealFunctions() {
  Real::initializer();
  funcInitialized = true;
}

__attribute__((constructor)) void initializer() {
  // Using globals to provide allocation
  // before initialized.
  // We can not use stack variable here since different process
  // may use this to share information.
  // initialize those library function entries.
//	fprintf(stderr, "initializer function now\n");
  if(!funcInitialized) {
		initRealFunctions();
	}

  if(!initialized) {
		// Now setup
    xrun::getInstance().initialize();
    initialized = true;
  }
}

__attribute__((destructor)) void finalizer() {
  xrun::getInstance().finalize();

  funcInitialized = false;
}

typedef int (*main_fn_t)(int, char**, char**);
main_fn_t real_main;

void exitfunc(void) {
	PRINT("in the end of exiting now\n");
}

// Doubletake's main function
int doubletake_main(int argc, char** argv, char** envp) {
  /******** Do doubletake initialization here (runs after ctors) *********/
//	printf("doubletake_main initializer\n");
	initializer();

	// Now start the first epoch
	xrun::getInstance().epochBegin();

	//PRINT("before actually call real_main function\n");
  // Call the program's main function
  return real_main(argc, argv, envp);
}

extern "C" int __libc_start_main(main_fn_t, int, char**, void (*)(), void (*)(), void (*)(), void*) __attribute__((weak, alias("doubletake_libc_start_main")));

extern "C" int doubletake_libc_start_main(main_fn_t main_fn, int argc, char** argv, void (*init)(), void (*fini)(), void (*rtld_fini)(), void* stack_end) {
  // Find the real __libc_start_main
  auto real_libc_start_main = (decltype(__libc_start_main)*)dlsym(RTLD_NEXT, "__libc_start_main");
  // Save the program's real main function
  real_main = main_fn;
  // Run the real __libc_start_main, but pass in doubletake's main function
  return real_libc_start_main(doubletake_main, argc, argv, init, fini, rtld_fini, stack_end);
}


// Temporary bump-pointer allocator for malloc() calls before DoubleTake is initialized
static void* tempmalloc(int size) {
  static char _buf[InitialMallocSize];
  static int _allocated = 0;

//	fprintf(stderr, "inside tempmalloc with size %d\n", size);
  if(_allocated + size > InitialMallocSize) {
    printf("Not enough space for tempmalloc");
    abort();
  } else {
    void* p = (void*)&_buf[_allocated];
    _allocated += size;
    return p;
  }
}

bool addThreadQuarantineList(void* ptr, size_t sz) {
  return xthread::getInstance().addQuarantineList(ptr, sz);
}

void* call_dlsym(void* handle, const char* funcname) {
  bool isCheckDisabled = false;
  if(initialized) {
    isCheckDisabled = xthread::isCheckDisabled();

    if(!isCheckDisabled) {
      xthread::disableCheck();
    }
  }

  void* p = dlsym(handle, funcname);

  if(initialized) {
    if(!isCheckDisabled) {
      xthread::enableCheck();
    }
  }
  return p;
}

// Memory management functions
extern "C" {
  void* xxmalloc(size_t sz) {
    void* ptr = NULL;

		//printf("xxmalloc sz %ld \n", sz);

		if(sz == 0) {
			sz = 1;
		}

    // Align the object size. FIXME for now, just use 16 byte alignment and min size.
    if(!initialized) {
    	if (sz < 16) {
      	sz = 16;
   		}
    	sz = (sz + 15) & ~15;
      ptr = tempmalloc(sz);
    } 
		else {
      ptr = xmemory::getInstance().malloc(sz);
    }
    if(ptr == NULL) {
    	fprintf(stderr, "Out of memory with initialized %d!\n", initialized);
      ::abort();
    }
		
		//fprintf(stderr, "MEMORY allocation with ptr %p to %lx\n", ptr, ((intptr_t)ptr + sz));
    return ptr;
  }

  void xxfree(void* ptr) {
    if(initialized && ptr) {
      xmemory::getInstance().free(ptr);
    }
  }

  size_t xxmalloc_usable_size(void* ptr) {
    if(initialized) {
      return xmemory::getInstance().getSize(ptr);
    }
    return 0;
  }

	void * xxrealloc(void * ptr, size_t sz) {
    if(initialized) {
      return xmemory::getInstance().realloc(ptr, sz);
		}
		else {
			void * newptr = tempmalloc(sz);
			return newptr;
		}
	}
}

// Thread functions
extern "C" {

#if !defined(DISABLE_THREADS)

  /// Threads's synchronization functions.
  // Mutex related functions
  int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    if(!funcInitialized) {
      initializer();
    }
    if(initialized)
      return xthread::getInstance().mutex_init(mutex, attr);
    return 0;
  }

  int pthread_mutex_lock(pthread_mutex_t* mutex) {
    //  PRINT("inside pthread_mutex_lock, line %d at %p. disablecheck %d!\n", __LINE__, mutex,
    // current->disablecheck);
    if(initialized) {
      return xthread::getInstance().mutex_lock(mutex);
    }
    return 0;
  }

  // FIXME: add support for trylock
  int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    // PRINT("inside pthread_mutex_lock, line %d!\n", __LINE__);
   	if(initialized) {
      return xthread::getInstance().mutex_trylock(mutex);
    }
    return 0;
  }

  int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    //   PRINT("inside pthread_mutex_unlock, line %d at %p. disablecheck %d!\n", __LINE__, mutex,
    // current->disablecheck);
    if(initialized) {
      xthread::getInstance().mutex_unlock(mutex);
    }

    return 0;
  }

  int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    return xthread::getInstance().mutex_destroy(mutex);
  }

  // Condition variable related functions
  int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* condattr) {
    if(initialized)
    	xthread::getInstance().cond_init(cond, condattr);
    return 0;
  }

  int pthread_cond_broadcast(pthread_cond_t* cond) {
    if(initialized)
      return xthread::getInstance().cond_broadcast(cond);
    return 0;
  }

  int pthread_cond_signal(pthread_cond_t* cond) {
    if(initialized)
      return xthread::getInstance().cond_signal(cond);
    return 0;
  }

  int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    if(initialized)
      return xthread::getInstance().cond_wait(cond, mutex);
    return 0;
  }

  int pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex,
			const struct timespec * abstime) 
		{
    assert(initialized == true);

    return xthread::getInstance().cond_timedwait(cond, mutex, abstime);
  }

  int pthread_cond_destroy(pthread_cond_t* cond) {
    if(initialized)
      xthread::getInstance().cond_destroy(cond);
    return 0;
  }

  // Barrier related functions
  int pthread_barrier_init(pthread_barrier_t* barrier, const pthread_barrierattr_t* attr,
			   unsigned int count) {
    return xthread::getInstance().barrier_init(barrier, attr, count);
  }

  int pthread_barrier_destroy(pthread_barrier_t* barrier) {
    return xthread::getInstance().barrier_destroy(barrier);
  }

  int pthread_barrier_wait(pthread_barrier_t* barrier) {
    if(initialized)
      return xthread::getInstance().barrier_wait(barrier);
    else
      return 0;
  }

  int pthread_cancel(pthread_t thread) { return xthread::getInstance().thread_cancel(thread); }

  int sched_yield() { return 0; }

  // FIXME
  void pthread_exit(void* /* value_ptr */) {
    // This should probably throw a special exception to be caught in spawn.
    abort();
    // xthread::getInstance().thread_exit(value_ptr);
  }

  int pthread_setconcurrency(int) { return 0; }

  pthread_t pthread_self() { return xthread::thread_self(); }

  int pthread_kill(pthread_t thread, int sig) {
    // FIX ME
    return xthread::getInstance().thread_kill(thread, sig);
  }

  int pthread_detach(pthread_t thread) { return xthread::getInstance().thread_detach(thread); }

  // int pthread_rwlock_destroy (pthread_rwlock_t * rwlock) NOTHROW {
  //   return 0;
  // }

  // int pthread_rwlock_init (pthread_rwlock_t * rwlock, const pthread_rwlockattr_t * attr) NOTHROW {
  //   return 0;
  // }

  // int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) NOTHROW {
  //   return 0;
  // }

  // int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) NOTHROW {
  //   return 0;
  // }

  // int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) NOTHROW {
  //   return 0;
  // }

  // int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) NOTHROW {
  //   return 0;
  // }

  // int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) NOTHROW{
  //   return 0;
  // }

  int pthread_attr_getstacksize(const pthread_attr_t*, size_t* s) {
    *s = 1048576UL; // really? FIX ME
    return 0;
  }

  /*
    int pthread_mutexattr_destroy (pthread_mutexattr_t *) { return 0; }
    int pthread_mutexattr_init (pthread_mutexattr_t *)    { return 0; }
    int pthread_mutexattr_settype (pthread_mutexattr_t *, int) { return 0; }
    int pthread_mutexattr_gettype (const pthread_mutexattr_t *, int *) { return 0; }
    int pthread_attr_setstacksize (pthread_attr_t *, size_t) { return 0; }
  */

  int pthread_create(pthread_t* tid, const pthread_attr_t* attr, void* (*start_routine)(void*),
		     void* arg) {
    return xthread::getInstance().thread_create(tid, attr, start_routine, arg);
  }

  int pthread_join(pthread_t tid, void** val) {
    xthread::getInstance().thread_join(tid, val);
    return 0;
  }

#endif
}

// System calls
extern "C" {

#if !defined(DISABLE_SYSCALLS)


/* 
	Avoid printing in the rollback phase.
  int puts(const char* s) {
    if(!global_isRollback()) {
      Real::puts(s);
    }
    return 0;
  }

  int printf(const char *format, ...) {
    PRINT("inside printf. global_isRolback() %d\n", global_isRollback());
    if(!global_isRollback()) {
      va_list ap;
      va_start(ap, format);
      vprintf(format, ap);
    }
  
    return 0;
  }

  int fprintf(FILE *stream, const char *format, ...) {
    if(!global_isRollback()) {
      va_list ap;
      va_start(ap, format);
      vfprintf(stream, format, ap);
    }
  
    return 0;
  }
*/
  
	// System calls related functions
  // SYSCall 1 - 10
  /*
    #define _SYS_read                                0
    #define _SYS_write                               1
    #define _SYS_open                                2
    #define _SYS_close                               3
    #define _SYS_stat                                4
    #define _SYS_fstat                               5
    #define _SYS_lstat                               6
    #define _SYS_poll                                7
    #define _SYS_lseek                               8
    #define _SYS_mmap                                9
    #define _SYS_mprotect                           10
    #define _SYS_munmap                             11
  */
#if 1
	ssize_t read(int fd, void* buf, size_t count) {
    ////fprintf(stderr, "**** read in doubletake at %d\n", __LINE__);
    //printf("**** read in doubletake at %d, initialized %d, real %p\n", __LINE__, initialized, Real::read);
    if(!initialized) {
      return Real::read(fd, buf, count);
    }

    return syscalls::getInstance().read(fd, buf, count);
  }

  ssize_t write(int fd, const void* buf, size_t count) {
    if(!initialized || fd == 1 || fd == 2) {
      return Real::write(fd, buf, count);
    } else {
      //  //fprintf(stderr, " write in doubletake at %d\n", __LINE__);
      return syscalls::getInstance().write(fd, buf, count);
    }
  }

  // int open(const char *pathname, int flags, mode_t mode) {
  int open(const char* pathname, int flags, ...) {
    int mode;

    if(flags & O_CREAT) {
      va_list arg;
      va_start(arg, flags);
      mode = va_arg(arg, mode_t);
      va_end(arg);
    } else {
      mode = 0;
    }
    //printf("**********open %s in doubletake at %d mod %d\n", pathname, __LINE__, mode);
    if(!initialized) {
      return Real::open(pathname, flags, mode);
    }
    return syscalls::getInstance().open(pathname, flags, mode);
  }


  int close(int fd) {
  //fprintf(stderr, "close fd %d ****** in libdoubletake\n", fd);
    if(!initialized) {
      return Real::close(fd);
    }
    return syscalls::getInstance().close(fd);
  }

  DIR* opendir(const char* name) { 
		return syscalls::getInstance().opendir(name); 
	}

  int closedir(DIR* dir) { 
		return syscalls::getInstance().closedir(dir); 
	}

	void seekdir(DIR *dir, long pos) { 
		syscalls::getInstance().seekdir(dir, pos); 
	}

	void rewinddir(DIR *dir) {
		syscalls::getInstance().rewinddir(dir); 
	}

	// We don't care about telldir and readdir;
  FILE* fopen(const char* filename, const char* modes) {
    //fprintf(stderr, "fopen in libdoubletake\n");
    if(!initialized) {
			if(!funcInitialized) {
			  initRealFunctions();
			}

      return Real::fopen(filename, modes);
    }
    return syscalls::getInstance().fopen(filename, modes);
  }

  // ostream.open actually calls this function
  FILE* fopen64(const char* filename, const char* modes) {
    if(!initialized) {
			if(!funcInitialized) {
			  initRealFunctions();
			}
      return Real::fopen64(filename, modes);
    }
   // fprintf(stderr, "fopen64 in libdoubletake\n");
    return syscalls::getInstance().fopen64(filename, modes);
  }
  
	FILE* freopen(const char* path, const char* mode, FILE* stream) {
  	fprintf(stderr, "freopen %d ****** in libdoubletake not supported\n", __LINE__);
    abort();
  }

	// We don't care about fread and fwrite since they won't call sockets.
  int fclose(FILE* fp) {
    if(!initialized) {
      return Real::fclose(fp);
    }
    return syscalls::getInstance().fclose(fp);
  }

  int fclose64(FILE* fp) { 
		return syscalls::getInstance().fclose(fp); 
	}
  /*
  // We don't need to handle the following system calls
  // since it doesn't matter whether those system calls
  // are called again or not.
  int stat(const char *path, struct stat *buf) {
  	return syscalls::getInstance().stat(path, buf);
  }

  int fstat(int filedes, struct stat *buf) {
  	return syscalls::getInstance().fstat(filedes, buf);
  }

  int lstat(const char *path, struct stat *buf) {
  	return syscalls::getInstance().lstat(path, buf);
  }


  int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  	return syscalls::getInstance().poll(fds, nfds, timeout);
  }

  */
  // Close current transaction since it is impossible to rollback.
  off_t lseek(int filedes, off_t offset, int whence) {
    //fprintf(stderr, "lseek in doubletake at %d. fd %d whence %d offset %ld\n", __LINE__, filedes, whence, offset);
    if(!initialized) {
      return Real::lseek(filedes, offset, whence);
    }
    return syscalls::getInstance().lseek(filedes, offset, whence);
  }
#endif 

  int mprotect(void* addr, size_t len, int prot) {
  //fprintf(stderr, "mprotect in doubletake at %d with current disablecheck %d\n", __LINE__, current->disablecheck);
    if(!initialized || current->disablecheck) {
      return Real::mprotect(addr, len, prot);
    }
    return syscalls::getInstance().mprotect(addr, len, prot);
  }
  
	void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset) {
  //fprintf(stderr, "*****mmap in doubletake at %d start %p fd %d length %lx\n", __LINE__, start, fd, length);
    if(!initialized || current->disablecheck) {
      return Real::mmap(start, length, prot, flags, fd, offset);
    }

    return syscalls::getInstance().mmap(start, length, prot, flags, fd, offset);
    // return Real::mmap(start, length, prot, flags, fd, offset);
  }


  int munmap(void* start, size_t length) {
    if(!initialized) {
      return Real::munmap(start, length);
    }
  //fprintf(stderr, "munmap in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().munmap(start, length);
  }

  /*
    #define _SYS_brk                                12
    #define _SYS_rt_sigaction                       13
    #define _SYS_rt_sigprocmask                     14
    #define _SYS_rt_sigreturn                       15
    #define _SYS_ioctl                              16
    #define _SYS_pread64                            17
    #define _SYS_pwrite64                           18
    #define _SYS_readv                              19
    #define _SYS_writev                             20
    #define _SYS_access                             21
    #define _SYS_pipe                               22
    #define _SYS_select                             23
    #define _SYS_sched_yield                        24
    #define _SYS_mremap                             25
    #define _SYS_msync                              26
    #define _SYS_mincore                            27
    #define _SYS_madvise                            28
    #define _SYS_shmget                             29
    #define _SYS_shmat                              30
    #define _SYS_shmctl                             31

  */
  int brk(void* end_data_segment) {
    ////fprintf(stderr, "brk in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().brk(end_data_segment);
  }

  int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
    if(current->disablecheck) {
      return Real::sigaction(signum, act, oldact);
    } else {
    //fprintf(stderr, "%d: sigaction in doubletake at %d. Disablecheck %d\n", __LINE__, getpid(), current->disablecheck);
      return syscalls::getInstance().sigaction(signum, act, oldact);
    }
  }

  int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
    if(current->disablecheck) {
      return Real::sigprocmask(how, set, oldset);
    } else {
    //fprintf(stderr, "sigprocmask in doubletake at %d\n", __LINE__);
      return syscalls::getInstance().sigprocmask(how, set, oldset);
    }
  }

  //  int sigreturn(unsigned long __unused) {
  //    return syscalls::getInstance().sigreturn(__unused);
  //  }

  // FIXME
  // int ioctl(int d, int request, ...) {
  //   va_list ap;
  //   va_start(ap, request);
  //   void * argp = va_arg(ap, void *);
  //   return syscalls::getInstance().ioctl(d, request, argp);
  // }

  // Since the offset will not changed at all, it is safe to omit this.
  // ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
  //   return syscalls::getInstance().pread(fd, buf, count, offset);
  // }

  ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    ////fprintf(stderr, "pwrite in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().pwrite(fd, buf, count, offset);
  }

  ssize_t readv(int fd, const struct iovec* vector, int count) {
    ////fprintf(stderr, "readv in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().readv(fd, vector, count);
  }

  ssize_t writev(int fd, const struct iovec* vector, int count) {
  //fprintf(stderr, "writev fd %d in doubletake at %d\n", fd, __LINE__);
    return syscalls::getInstance().writev(fd, vector, count);
  }

  // Check permission, no need to intercept
  // int access(const char *pathname, int mode){
  //   return syscalls::getInstance().access(pathname, mode);
  // }

  int pipe(int filedes[2]) {
  //fprintf(stderr, "pipe in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().pipe(filedes);
  }

  // No need to handle this
  // int select(int nfds, fd_set *readfds, fd_set *writefds,
  //            fd_set *exceptfds, struct timeval *timeout){
  //   return syscalls::getInstance().select(nfds, readfds, writefds, exceptfds, timeout);
  // }

  // Tonngping: Record this
  void* mremap(void* old_address, size_t old_size, size_t new_size, int flags, ...) {
  //fprintf(stderr, "mremap in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().mremap(old_address, old_size, new_size, flags);
  }

  // No need to do anything
  // int msync(void *start, size_t length, int flags){
  //   return syscalls::getInstance().msync(start, length, flags);
  // }

  int mincore(void* /* start */, size_t /* length */, unsigned char* /* vec */) {
    // FIXME later
    assert(0);
    return 0;
    //    return syscalls::getInstance().mincore(start, length, vec);
  }

  // int madvise(void *start, size_t length, int advice){
  // //fprintf(stderr, "madvise in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().madvise(start, length, advice);
  // }

  int shmget(key_t key, size_t size, int shmflg) {
    ////fprintf(stderr, "shmget in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().shmget(key, size, shmflg);
  }

  void* shmat(int shmid, const void* shmaddr, int shmflg) {
  //fprintf(stderr, "shmat in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().shmat(shmid, shmaddr, shmflg);
  }

  int shmctl(int /* shmid */, int /* cmd */, struct shmid_ds* /* buf */) {
  //fprintf(stderr, "shmctl is not supported now\n");
    abort();
    return 0;
    // return syscalls::getInstance().shmctl(shmid, cmd, buf);
  }

  /*
    #define _SYS_dup                                32
    #define _SYS_dup2                               33
    #define _SYS_pause                              34
    #define _SYS_nanosleep                          35
    #define _SYS_getitimer                          36
    #define _SYS_alarm                              37
    #define _SYS_setitimer                          38
    #define _SYS_getpid                             39
    #define _SYS_sendfile                           40
    #define _SYS_socket                             41
    #define _SYS_connect                            42
    #define _SYS_accept                             43
    #define _SYS_sendto                             44
    #define _SYS_recvfrom                           45
    #define _SYS_sendmsg                            46
    #define _SYS_recvmsg                            47
    #define _SYS_shutdown                           48
    #define _SYS_bind                               49
    #define _SYS_listen                             50
    #define _SYS_getsockname                        51
    #define _SYS_getpeername                        52
  */

  int dup(int oldfd) {
    ////fprintf(stderr, "dup in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().dup(oldfd);
  }

  int dup2(int oldfd, int newfd) {
    ////fprintf(stderr, "dup2 in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().dup2(oldfd, newfd);
  }

  // int pause(){
  //   return syscalls::getInstance().pause();
  // }

  // int nanosleep(const struct timespec *req, struct timespec *rem){
  //   return syscalls::getInstance().nanosleep(req, rem);
  // }

  // int getitimer(int which, struct itimerval *value){
  //   return syscalls::getInstance().getitimer(which, value);
  // }

  unsigned int alarm(unsigned int seconds) {
    ////fprintf(stderr, "alarm in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().alarm(seconds);
  }

  int setitimer(int which, const struct itimerval* value, struct itimerval* ovalue) {
    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setitimer(which, value, ovalue);
  }

  // pid_t getpid(){
  //  return syscalls::getInstance().getpid();
  // }

  ssize_t sendfile(int out_fd, int in_fd, off_t* offset, size_t count) {
    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sendfile(out_fd, in_fd, offset, count);
  }

  int socket(int domain, int type, int protocol) {
    ////fprintf(stderr, "%d: in doubletake at %d\n", getpid(), __LINE__);
    return syscalls::getInstance().socket(domain, type, protocol);
  }

  int connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen) {
    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().connect(sockfd, serv_addr, addrlen);
  }

  int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().accept(sockfd, addr, addrlen);
  }

  ssize_t sendto(int s, const void* buf, size_t len, int flags, const struct sockaddr* to,
		 socklen_t tolen) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().sendto(s, buf, len, flags, to, tolen);
  }

  ssize_t recvfrom(int s, void* buf, size_t len, int flags, struct sockaddr* from,
		   socklen_t* fromlen) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().recvfrom(s, buf, len, flags, from, fromlen);
  }

  ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sendmsg(s, msg, flags);
  }

  // Complicated, unsupported now FIXME
  ssize_t recvmsg(int /* s */, struct msghdr* /* msg */, int /* flags */) {
  //fprintf(stderr, "recvmsg is not supported now\n");
    return 0;
    //    return syscalls::getInstance().recvmsg(s, msg, flags);
  }

  int shutdown(int /* s */, int /* how */) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    abort();
    // FIXME
    return 0;
  }

  int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().bind(sockfd, my_addr, addrlen);
  }

  int listen(int sockfd, int backlog) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().listen(sockfd, backlog);
  }

  // No need to handle this.
  // int getsockname(int s, struct sockaddr *name, socklen_t *namelen){
  //   return syscalls::getInstance().getsockname(s, name, namelen);
  // }

  // int getpeername(int s, struct sockaddr *name, socklen_t *namelen){
  //   return syscalls::getInstance().getpeername(s, name, namelen);
  // }

  int socketpair(int d, int type, int protocol, int sv[2]) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().socketpair(d, type, protocol, sv);
  }

  int setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().setsockopt(s, level, optname, optval, optlen);
  }

  // int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen){
  //   return syscalls::getInstance().getsockopt(s, level, optname, optval, optlen);
  // }

  // int __clone(int (*fn)(void *), void *child_stack, int flags, void *arg, pid_t *pid, struct
  // user_desc *tls, pid_t *ctid) {
  //   printf("inside clone\n");
  //   return syscalls::getInstance().__clone(fn, child_stack, flags, arg, pid, tls, ctid);
  //
  // }

  //  pid_t fork(){

  //  pid_t vfork(){

  int execve(const char* filename, char* const argv[], char* const envp[]) {

  //fprintf(stderr, " in doubletake at %d. filename %s\n", __LINE__, filename);
    if(current->disablecheck) {
      return Real::execve(filename, argv, envp);
    } else {
      return syscalls::getInstance().execve(filename, argv, envp);
    }
  }

  // void exit(int status){
  //   //FIXME
  //   syscalls::getInstance().exit(status);
  // }

  // pid_t wait4(pid_t pid, void *status, int options,
  //       struct rusage *rusage){
  //
  //   return syscalls::getInstance().wait4(pid, status, options, rusage);
  // }

  // int kill(pid_t pid, int sig){
  //   // FIXME
  //   return syscalls::getInstance().kill(pid, sig);
  // }

  // int uname(struct utsname *buf){
  //   return syscalls::getInstance().uname(buf);
  // }

  /*

    #define _SYS_semget                             64
    #define _SYS_semop                              65
    #define _SYS_semctl                             66
    #define _SYS_shmdt                              67
    #define _SYS_msgget                             68
    #define _SYS_msgsnd                             69
    #define _SYS_msgrcv                             70
    #define _SYS_msgctl                             71
    #define _SYS_fcntl                              72
    #define _SYS_flock                              73
    #define _SYS_fsync                              74
    #define _SYS_fdatasync                          75
    #define _SYS_truncate                           76
    #define _SYS_ftruncate                          77
    #define _SYS_getdents                           78
    #define _SYS_getcwd                             79
    #define _SYS_chdir                              80

  */

  // Tongping
  int semget(key_t key, int nsems, int semflg) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().semget(key, nsems, semflg);
  }

  int semop(int semid, struct sembuf* sops, size_t nsops) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().semop(semid, sops, nsops);
  }

  int semctl(int /* semid */, int /* semnum */, int /* cmd */, ...) {
    // FIXME
  //fprintf(stderr, "semctl is not supported now\n");
    abort();
    return 0;
    // syscalls::getInstance().semctl(semid, semnum, cmd);
  }

  // int fcntl(int fd, int cmd, struct flock *lock, ...){
  int fcntl(int fd, int cmd, ...) {
    va_list ap;
    va_start(ap, cmd);
    long arg = va_arg(ap, long);
    va_end(ap);
    ////fprintf(stderr, " in doubletake at %d cmd %d))))))))))\n", __LINE__, cmd);
    return syscalls::getInstance().fcntl(fd, cmd, arg);
  }

  int flock(int fd, int operation) {
    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().flock(fd, operation);
  }

  int fsync(int fd) {
    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fsync(fd);
  }

  int fdatasync(int fd) {
    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fdatasync(fd);
  }
  // Tongping
  int truncate(const char* path, off_t length) {

    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().truncate(path, length);
  }

  int ftruncate(int fd, off_t length) {

    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().ftruncate(fd, length);
  }

  // int getdents(unsigned int fd, struct dirent *dirp, unsigned int count){
  //
  //   return syscalls::getInstance().getdents(fd, dirp, count);
  // }

  // char * getcwd(char *buf, size_t size){
  //   return syscalls::getInstance().getcwd(buf, size);
  // }

  int chdir(const char* path) {
    ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().chdir(path);
  }

  int fchdir(int fd) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fchdir(fd);
  }

  /*
    #define _SYS_rename                             82
    #define _SYS_mkdir                              83
    #define _SYS_rmdir                              84
    #define _SYS_creat                              85
    #define _SYS_link                               86
    #define _SYS_unlink                             87
    #define _SYS_symlink                            88
    #define _SYS_readlink                           89
    #define _SYS_chmod                              90
    #define _SYS_fchmod                             91
    #define _SYS_chown                              92
    #define _SYS_fchown                             93
    #define _SYS_lchown                             94
    #define _SYS_umask                              95
    #define _SYS_gettimeofday                       96
    #define _SYS_getrlimit                          97
    #define _SYS_getrusage                          98
    #define _SYS_sysinfo                            99
    #define _SYS_times                             100
    #define _SYS_ptrace                            101
    #define _SYS_getuid                            102
    #define _SYS_syslog                            103
    #define _SYS_getgid                            104
  */

  int rename(const char* oldpath, const char* newpath) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().rename(oldpath, newpath);
  }

  int mkdir(const char* pathname, mode_t mode) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().mkdir(pathname, mode);
  }

  int rmdir(const char* pathname) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().rmdir(pathname);
  }

  int creat(const char* pathname, mode_t mode) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //fprintf(stderr, "&&&&&&&&&&&&&&&creat&&&&&&&&&&&&&&&&& is not supported.\n");
    return syscalls::getInstance().creat(pathname, mode);
  }

  int link(const char* oldpath, const char* newpath) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().link(oldpath, newpath);
  }

  int unlink(const char* pathname) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().unlink(pathname);
  }

  int symlink(const char* oldpath, const char* newpath) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().symlink(oldpath, newpath);
  }

  ssize_t readlink(const char* path, char* buf, size_t bufsize) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().readlink(path, buf, bufsize);
  }

  int chmod(const char* path, mode_t mode) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().chmod(path, mode);
  }

  int fchmod(int fildes, mode_t mode) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fchmod(fildes, mode);
  }

  int chown(const char* path, uid_t owner, gid_t group) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().chown(path, owner, group);
  }

  int fchown(int fd, uid_t owner, gid_t group) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fchown(fd, owner, group);
  }

  int lchown(const char* path, uid_t owner, gid_t group) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().lchown(path, owner, group);
  }

  // Tongping
  mode_t umask(mode_t mask) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().umask(mask);
  }

  int gettimeofday(struct timeval* tv, struct timezone* tz) {
    return syscalls::getInstance().gettimeofday(tv, tz);
  }

  // Initialization code should call this function, this cannot be intercepted.
  // int getrlimit(int resource, struct rlimit *rlim){
  //
  //   return syscalls::getInstance().getrlimit(resource, rlim);
  // }

  // int getrusage(int who, struct rusage *usage){
  //   return syscalls::getInstance().getrusage(who, usage);
  // }

  // int sysinfo(struct sysinfo *info){
  //   return syscalls::getInstance().sysinfo(info);
  // }

  clock_t times(struct tms* buf) {
  //fprintf(stderr, "calling times now\n");
    //   return 0;
    return syscalls::getInstance().times(buf);
  }

  // long ptrace(enum __ptrace_request request, pid_t pid,
  //            void *addr, void *data){
  //  return 0;
  // }

  // uid_t getuid(){
  //
  //   return syscalls::getInstance().getuid();
  // }

  void syslog(int pri, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    syscalls::getInstance().vsyslog(pri, fmt, args);
    va_end(args);
  }

  void vsyslog(int pri, const char* fmt, va_list args) {
    syscalls::getInstance().vsyslog(pri, fmt, args);
  }

  /*
    #define _SYS_setuid                            105
    #define _SYS_setgid                            106
    #define _SYS_geteuid                           107
    #define _SYS_getegid                           108
    #define _SYS_setpgid                           109
    #define _SYS_getppid                           110
    #define _SYS_getpgrp                           111
    #define _SYS_setsid                            112
    #define _SYS_setreuid                          113
    #define _SYS_setregid                          114
    #define _SYS_getgroups                         115
    #define _SYS_setgroups                         116
    #define _SYS_setresuid                         117
    #define _SYS_getresuid                         118
    #define _SYS_setresgid                         119
    #define _SYS_getresgid                         120
    #define _SYS_getpgid                           121
    #define _SYS_setfsuid                          122
    #define _SYS_setfsgid                          123
    #define _SYS_getsid                            124
    #define _SYS_capget                            125
    #define _SYS_capset                            126
    #define _SYS_rt_sigpending                     127
  */

  // gid_t getgid(){
  //   return syscalls::getInstance().getgid();
  // }

  int setuid(uid_t uid) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setuid(uid);
  }
  int setgid(gid_t gid) { return syscalls::getInstance().setgid(gid); }

  // uid_t geteuid(){
  //   return syscalls::getInstance().geteuid();
  // }

  // gid_t getegid(){
  //   return syscalls::getInstance().getegid();
  // }

  int setpgid(pid_t pid, pid_t pgid) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setpgid(pid, pgid);
  }

  // pid_t getppid(){
  //   return syscalls::getInstance().getppid();
  // }

  // pid_t getpgrp(){
  //   return syscalls::getInstance().getpgrp();
  // }

  pid_t setsid() {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setsid();
  }

  int setreuid(uid_t ruid, uid_t euid) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setreuid(ruid, euid);
  }

  int setregid(gid_t rgid, gid_t egid) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setregid(rgid, egid);
  }

  // int getgroups(int size, gid_t list[]){
  //   return syscalls::getInstance().setgroups(size, list);
  // }

  int setgroups(size_t size, const gid_t* list) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setgroups(size, list);
  }

  int setresuid(uid_t ruid, uid_t euid, uid_t suid) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setresuid(ruid, euid, suid);
  }

  // int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid){
  //   return syscalls::getInstance().getresuid(ruid, euid, suid);
  // }

  int setresgid(gid_t rgid, gid_t egid, gid_t sgid) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setresgid(rgid, egid, sgid);
  }

  // int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid){
  //   return syscalls::getInstance().getresgid(rgid, egid, sgid);
  // }

  // pid_t getpgid(pid_t pid){
  //   return syscalls::getInstance().getpgid(pid);
  // }

  int setfsuid(uid_t fsuid) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setfsuid(fsuid);
  }

  int setfsgid(uid_t fsgid) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setfsgid(fsgid);
  }

  /*
    #define _SYS_getsid                            124
    #define _SYS_capget                            125
    #define _SYS_capset                            126
    #define _SYS_rt_sigpending                     127
    #define _SYS_rt_sigtimedwait                   128
    #define _SYS_rt_sigqueueinfo                   129
    #define _SYS_rt_sigsuspend                     130
    #define _SYS_sigaltstack                       131
    #define _SYS_utime                             132
    #define _SYS_mknod                             133
    #define _SYS_uselib                            134
    #define _SYS_personality                       135
    #define _SYS_ustat                             136
    #define _SYS_statfs                            137
    #define _SYS_fstatfs                           138
    #define _SYS_sysfs                             139
  */

  // pid_t getsid(pid_t pid){
  //   return syscalls::getInstance().getsid(pid);
  // }

  // int sigpending(sigset_t *set){
  //   return syscalls::getInstance().sigpending(set);
  // }

  int sigtimedwait(const sigset_t* set, siginfo_t* info, const struct timespec* timeout) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sigtimedwait(set, info, timeout);
  }

  int sigsuspend(const sigset_t* mask) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);

    return syscalls::getInstance().sigsuspend(mask);
  }

  int sigaltstack(const stack_t* ss, stack_t* oss) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sigaltstack(ss, oss);
  }

  int utime(const char* filename, const struct utimbuf* buf) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().utime(filename, buf);
  }

  // int mknod(const char *pathname, __mode_t mode, __dev_t dev){
  //   return syscalls::getInstance().mknod(pathname, mode, dev);
  // }

  // No libc wrapper
  // int uselib(const char *library){
  // //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().uselib(library);
  // }

  int personality(unsigned long persona) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().personality(persona);
  }

  // int ustat(dev_t dev, struct ustat *ubuf){
  //   return syscalls::getInstance().ustat(dev, ubuf);
  // }

  // int statfs(const char *path, struct statfs *buf){
  //   return syscalls::getInstance().statfs(path, buf);
  // }

  // int fstatfs(int fd, struct statfs *buf){
  //   return syscalls::getInstance().fstatfs(fd, buf);
  // }

  // int sysfs(int option, unsigned int fs_index, char *buf){
  //   return syscalls::getInstance().sysfs(option, fs_index, buf);
  // }

  /*
    #define _SYS_getpriority                       140
    #define _SYS_setpriority                       141
    #define _SYS_sched_setparam                    142
    #define _SYS_sched_getparam                    143
    #define _SYS_sched_setscheduler                144
    #define _SYS_sched_getscheduler                145
    #define _SYS_sched_get_priority_max            146
    #define _SYS_sched_get_priority_min            147
    #define _SYS_sched_rr_get_interval             148
    #define _SYS_mlock                             149
    #define _SYS_munlock                           150
    #define _SYS_mlockall                          151
    #define _SYS_munlockall                        152
    #define _SYS_vhangup                           153
    #define _SYS_modify_ldt                        154
    #define _SYS_pivot_root                        155
    #define _SYS__sysctl                           156
    #define _SYS_prctl                             157
    #define _SYS_arch_prctl                        158
    #define _SYS_adjtimex                          159
    #define _SYS_setrlimit                         160
    #define _SYS_chroot                            161
    #define _SYS_sync                              162
    #define _SYS_acct                              163
  */

  // int getpriority(int which, id_t who){
  //   return syscalls::getInstance().getpriority(which, who);
  // }

  int setpriority(__priority_which_t which, id_t who, int prio) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setpriority(which, who, prio);
  }

  int sched_setparam(pid_t pid, const struct sched_param* param) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sched_setparam(pid, param);
  }

  // int sched_getparam(pid_t pid, struct sched_param *param){
  //   return syscalls::getInstance().sched_getparam(pid, param);
  // }

  int sched_setscheduler(pid_t pid, int policy, const struct sched_param* param) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sched_setscheduler(pid, policy, param);
  }

  // int sched_getscheduler(pid_t pid){
  //   return syscalls::getInstance().sched_getscheduler(pid);
  // }

  // int sched_get_priority_max(int policy){
  //   return syscalls::getInstance().sched_get_priority_max(policy);
  // }

  // int sched_get_priority_min(int policy){
  //   return syscalls::getInstance().sched_get_priority_min(policy);
  // }

  // int sched_rr_get_interval(pid_t pid, struct timespec *tp){
  //   return syscalls::getInstance().sched_rr_get_interval(pid, tp);
  // }

  int mlock(const void* addr, size_t len) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().mlock(addr, len);
  }

  int munlock(const void* addr, size_t len) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().munlock(addr, len);
  }

  int mlockall(int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().mlockall(flags);
  }

  int munlockall() {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().munlockall();
  }

  int vhangup() {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().vhangup();
  }

  int modify_ldt(int /* func */, void* /* ptr */, unsigned long /* bytecount */) {
  //fprintf(stderr, "modify_ldt is not supported now\n"); // FIXME
    return 0;
    //  return syscalls::getInstance().modify_ldt(func, ptr, bytecount);
  }

  // No libc wrapper
  // int pivot_root(const char *new_root, const char *put_old){
  // //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().pivot_root(new_root, put_old);
  // }

  int prctl(int /* option */, ...) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    // No vararg passing. Have to match on option //FIXME
    abort();
    // return syscalls::getInstance().prctl(option, arg2, arg3, arg4, arg5);
  }

  // no libc wrapper
  // int arch_prctl(int code, unsigned long addr) {
  // //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().arch_prctl(code, addr);
  // }

  int adjtimex(struct timex* buf) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().adjtimex(buf);
  }

  int setrlimit(int resource, const struct rlimit* rlim) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setrlimit(resource, rlim);
  }

  int chroot(const char* path) {
    // FIXME
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().chroot(path);
  }

  void sync() {
    // FIXME
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sync();
  }

  int acct(const char* filename) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().acct(filename);
  }

  /*
    #define _SYS_settimeofday                      164
    #define _SYS_mount                             165
    #define _SYS_umount2                           166
    #define _SYS_swapon                            167
    #define _SYS_swapoff                           168
    #define _SYS_reboot                            169
    #define _SYS_sethostname                       170
    #define _SYS_setdomainname                     171
    #define _SYS_iopl                              172
    #define _SYS_ioperm                            173
    #define _SYS_create_module                     174
    #define _SYS_init_module                       175
    #define _SYS_delete_module                     176
    #define _SYS_get_kernel_syms                   177
    #define _SYS_query_module                      178
    #define _SYS_quotactl                          179
    #define _SYS_nfsservctl                        180
    #define _SYS_getpmsg                           181  // reserved for LiS/STREAMS
    #define _SYS_putpmsg                           182  // reserved for LiS/STREAMS
    #define _SYS_afs_syscall                       183  // reserved for AFS
    #define _SYS_tuxcall          184 // reserved for tux
    #define _SYS_security     185
    #define _SYS_gettid   186
    #define _SYS_readahead    187
  */

  int settimeofday(const struct timeval* tv, const struct timezone* tz) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().settimeofday(tv, tz);
  }

  int mount(const char* source, const char* target, const char* filesystemtype,
	    unsigned long mountflags, const void* data) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().mount(source, target, filesystemtype, mountflags, data);
  }

  int umount2(const char* target, int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().umount2(target, flags);
  }

  int swapon(const char* path, int swapflags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().swapon(path, swapflags);
  }

  int swapoff(const char* path) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().swapoff(path);
  }

  int reboot(int howto) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().reboot(howto);
  }

  int sethostname(const char* name, size_t len) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sethostname(name, len);
  }

  int setdomainname(const char* name, size_t len) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setdomainname(name, len);
  }

  int iopl(int level) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().iopl(level);
  }

  int ioperm(unsigned long from, unsigned long num, int turn_on) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().ioperm(from, num, turn_on);
  }

  // pid_t gettid(){
  //   return syscalls::getInstance().gettid();
  // }

  /*
    #define _SYS_readahead    187
    #define _SYS_setxattr   188
    #define _SYS_lsetxattr    189
    #define _SYS_fsetxattr    190
    #define _SYS_getxattr   191
    #define _SYS_lgetxattr    192
    #define _SYS_fgetxattr    193
    #define _SYS_listxattr    194
    #define _SYS_llistxattr   195
    #define _SYS_flistxattr   196
    #define _SYS_removexattr  197
    #define _SYS_lremovexattr 198
    #define _SYS_fremovexattr 199
    #define _SYS_tkill  200
    #define _SYS_time      201
    #define _SYS_futex     202
    #define _SYS_sched_setaffinity    203
    #define _SYS_sched_getaffinity     204
    #define _SYS_set_thread_area  205
    #define _SYS_io_setup 206
    #define _SYS_io_destroy 207
    #define _SYS_io_getevents 208
    #define _SYS_io_submit  209
    #define _SYS_io_cancel  210
  */
  ssize_t readahead(int fd, __off64_t offset, size_t count) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().readahead(fd, offset, count);
  }

  int setxattr(const char* path, const char* name, const void* value, size_t size, int flags) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().setxattr(path, name, value, size, flags);
  }

  int lsetxattr(const char* path, const char* name, const void* value, size_t size, int flags) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().lsetxattr(path, name, value, size, flags);
  }

  int fsetxattr(int filedes, const char* name, const void* value, size_t size, int flags) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fsetxattr(filedes, name, value, size, flags);
  }

  // ssize_t getxattr (const char *path, const char *name, void *value, size_t size){
  //   return syscalls::getInstance().getxattr(path, name, value, size);
  // }

  // ssize_t lgetxattr (const char *path, const char *name, void *value, size_t size){
  //   return syscalls::getInstance().lgetxattr(path, name, value, size);
  // }

  // ssize_t fgetxattr (int filedes, const char *name, void *value, size_t size){
  //   return syscalls::getInstance().fgetxattr(filedes, name, value, size);
  // }

  // ssize_t listxattr (const char *path, char *list, size_t size){
  //   return syscalls::getInstance().listxattr(path, list, size);
  // }

  // ssize_t llistxattr (const char *path, char *list, size_t size){
  //   return syscalls::getInstance().llistxattr(path, list, size);
  // }

  // ssize_t flistxattr (int filedes, char *list, size_t size){
  //   return syscalls::getInstance().flistxattr(filedes, list, size);
  // }

  int removexattr(const char* path, const char* name) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().removexattr(path, name);
  }

  int lremovexattr(const char* path, const char* name) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().lremovexattr(path, name);
  }

  int fremovexattr(int filedes, const char* name) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fremovexattr(filedes, name);
  }

  // int tkill(int tid, int sig){
  //   // FIXME
  //   return 0;
  //   //return syscalls::getInstance().tkill(tid, sig);
  // }

  time_t time(time_t* t) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().time(t);
  }

  int futex(int* /* uaddr */, int /* op */, int /* val */,
	    const struct timespec* /* timeout */, int* /* uaddr2 */, int /* val3 */) {
  //fprintf(stderr, "futex is not supported\n"); // FIXME
    return 0;
    //    return syscalls::getInstance().futex(uaddr, op, val, timeout, uaddr2, val3);
  }

  int sched_setaffinity(__pid_t pid, size_t cpusetsize, const cpu_set_t* mask) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sched_setaffinity(pid, cpusetsize, mask);
  }

  // int sched_getaffinity(__pid_t pid, size_t cpusetsize, cpu_set_t *mask){
  //   return syscalls::getInstance().sched_getaffinity(pid, cpusetsize, mask);
  // }

  // set_thread_area doesn't have a libc wrapper
  /*int set_thread_area (struct user_desc *u_info){

//fprintf(stderr, " in doubletake at %d\n", __LINE__);
  return syscalls::getInstance().set_thread_area(u_info);
  }*/

  // int io_setup (int maxevents, io_context_t *ctxp){
  //   return syscalls::getInstance().io_setup(maxevents, ctxp);
  // }

  // int io_destroy (io_context_t ctx){
  //   return syscalls::getInstance().io_destroy(ctx);
  // }

  // long io_getevents (aio_context_t ctx_id, long min_nr, long nr,
  //                    struct io_event *events, struct timespec *timeout){
  //   return syscalls::getInstance().io_getevents(ctx_id, min_nr, nr, events, timeout);
  // }

  // long io_submit (aio_context_t ctx_id, long nr, struct iocb **iocbpp){
  //   return syscalls::getInstance().io_submit(ctx_id, nr, iocbpp);
  // }

  // long io_cancel (aio_context_t ctx_id, struct iocb *iocb, struct io_event *result){
  //   return syscalls::getInstance().io_cancel(ctx_id, nr, iocbpp);
  // }

  /*
    #define _SYS_get_thread_area  211
    #define _SYS_lookup_dcookie 212
    #define _SYS_epoll_create 213
    #define _SYS_epoll_ctl_old  214
    #define _SYS_epoll_wait_old 215
    #define _SYS_remap_file_pages 216
    #define _SYS_getdents64 217
    #define _SYS_set_tid_address  218
    #define _SYS_restart_syscall  219
    #define _SYS_semtimedop   220
    #define _SYS_fadvise64    221
    #define _SYS_timer_create   222
    #define _SYS_timer_settime    223
    #define _SYS_timer_gettime    224
    #define _SYS_timer_getoverrun   225
    #define _SYS_timer_delete 226
    #define _SYS_clock_settime  227
    #define _SYS_clock_gettime  228
    #define _SYS_clock_getres 229
    #define _SYS_clock_nanosleep  230
    #define _SYS_exit_group   231
    #define _SYS_epoll_wait   232
    #define _SYS_epoll_ctl    233
    #define _SYS_tgkill   234
  */

  // int get_thread_area(struct user_desc *u_info){
  //   return syscalls::getInstance().get_thread_area(u_info);
  //
  // }

  // int lookup_dcookie(u64 cookie, char * buffer, size_t len){
  //  return syscalls::getInstance().lookup_dcookie(cookie, buffer, len);
  // }

  int epoll_create(int size) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().epoll_create(size);
  }

  int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().epoll_ctl(epfd, op, fd, event);
  }

  int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().epoll_wait(epfd, events, maxevents, timeout);
  }

  int remap_file_pages(void* start, size_t size, int prot, size_t pgoff, int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().remap_file_pages(start, size, prot, pgoff, flags);
  }

  // No libc wrapper
  // long set_tid_address (int *tidptr){
  // //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().set_tid_address(tidptr);
  // }

  // No libc wrapper
  // long restart_syscall(){
  // //fprintf(stderr, "sys_restart_syscall is not supported by doubletake yet\n");
  //   return 0;
  // //    return syscalls::getInstance().sys_restart_syscall();
  // }

  int semtimedop(int semid, struct sembuf* sops, size_t nsops, const struct timespec* timeout) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().semtimedop(semid, sops, nsops, timeout);
  }

  int posix_fadvise64(int fs, loff_t offset, loff_t len, int advice) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().posix_fadvise64(fs, offset, len, advice);
  }

  int timer_create(clockid_t which_clock, struct sigevent* timer_event_spec,
		   timer_t* created_timer_id) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().timer_create(which_clock, timer_event_spec, created_timer_id);
  }

  int timer_settime(timer_t timer_id, int flags, const struct itimerspec* new_setting,
		    struct itimerspec* old_setting) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().timer_settime(timer_id, flags, new_setting, old_setting);
  }

  int timer_gettime(timer_t timer_id, struct itimerspec* setting) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().timer_gettime(timer_id, setting);
  }

  int timer_getoverrun(timer_t timer_id) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().timer_getoverrun(timer_id);
  }

  int timer_delete(timer_t timer_id) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().timer_delete(timer_id);
  }

  int clock_settime(clockid_t which_clock, const struct timespec* tp) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().clock_settime(which_clock, tp);
  }

  int clock_gettime(clockid_t which_clock, struct timespec* tp) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().clock_gettime(which_clock, tp);
  }

  int clock_getres(clockid_t which_clock, struct timespec* tp) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().clock_getres(which_clock, tp);
  }

  // long clock_nanosleep (clockid_t which_clock, int flags,
  //                         const struct timespec *rqtp, struct timespec *rmtp){
  //   return syscalls::getInstance().clock_nanosleep(which_clock, flags, rqtp, rmtp);
  // }

  // No libc wrapper
  /*void exit_group(int status){
//fprintf(stderr, " in doubletake at %d\n", __LINE__);
  return syscalls::getInstance().exit_group(status);
  }*/

  // long tgkill (int tgid, int pid, int sig){
  //   return syscalls::getInstance().tgkill(tgid, pid, sig);
  // }

  /*
    #define _SYS_utimes   235
    #define _SYS_vserver    236
    #define _SYS_mbind    237
    #define _SYS_set_mempolicy  238
    #define _SYS_get_mempolicy  239
    #define _SYS_mq_open    240
    #define _SYS_mq_unlink    241
    #define _SYS_mq_timedsend   242
    #define _SYS_mq_timedreceive  243
    #define _SYS_mq_notify    244
    #define _SYS_mq_getsetattr  245
    #define _SYS_kexec_load   246
    #define _SYS_waitid   247
    #define _SYS_add_key    248
    #define _SYS_request_key  249
    #define _SYS_keyctl   250
    #define _SYS_ioprio_set   251
    #define _SYS_ioprio_get   252
    #define _SYS_inotify_init 253
    #define _SYS_inotify_add_watch  254
    #define _SYS_inotify_rm_watch 255
    #define _SYS_migrate_pages  256
    #define _SYS_openat   257
    #define _SYS_mkdirat    258
  */
  int utimes(const char* filename, const struct timeval times[2]) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().utimes(filename, times);
  }

  // mqd_t mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr){
  //   return syscalls::getInstance().mq_open(name, oflag, mode, attr);
  // }

  // mqd_t mq_unlink(const char *name){
  //   return syscalls::getInstance().mq_unlink(name);
  // }

  // mqd_t mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio, const
  // struct timespec *abs_timeout){
  //   return syscalls::getInstance().mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
  // }

  // mqd_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio, const
  // struct timespec *abs_timeout){
  //   return syscalls::getInstance().mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
  // }

  // mqd_t mq_notify(mqd_t mqdes, const struct sigevent *notification){
  //   return syscalls::getInstance().mq_notify(mqdes, notification);
  // }

  // mqd_t mq_getsetattr(mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr){
  //   return syscalls::getInstance().mq_getsetattr(mqdes, newattr, oldattr);
  // }

  // int kexec_load(void* entry, size_t nr_segments, struct kexec_segment* segments, unsigned long int
  // flags) {
  //   return syscalls::getInstance().kexec_load(entry, nr_segments, segments, flags);
  // }

  int waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().waitid(idtype, id, infop, options);
  }

  // key_serial_t add_key(const char *type, const char *description,
  //                      const void *payload, size_t plen, key_serial_t keyring) {
  //   return syscalls::getInstance().add_key(type, description, payload, plen, keyring);
  // }

  // key_serial_t request_key(const char *type, const char *description,
  //                          const char *callout_info, key_serial_t keyring){
  //   return syscalls::getInstance().request_key(type, description, callout_info, keyring);
  // }

  // long keyctl(int cmd, ...){
  //   // FIXME
  // //fprintf(stderr, "keyctl is not supported now\n");
  //   return 0;
  // }

  // No libc wrappers
  // int ioprio_get(int which, int who){
  //   return syscalls::getInstance().ioprio_get(which, who);
  // }

  // int ioprio_set(int which, int who, int ioprio){
  // //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().ioprio_set(which, who, ioprio);
  // }

  // int inotify_init(){
  //   return syscalls::getInstance().inotify_init();
  // }

  int inotify_add_watch(int fd, const char* pathname, uint32_t mask) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().inotify_add_watch(fd, pathname, mask);
  }

  int inotify_rm_watch(int fd, int wd) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().inotify_rm_watch(fd, wd);
  }

  /*
    #define _SYS_openat   257
    #define _SYS_mkdirat    258
    #define _SYS_mknodat    259
    #define _SYS_fchownat   260
    #define _SYS_futimesat    261
    #define _SYS_newfstatat   262
    #define _SYS_unlinkat   263
    #define _SYS_renameat   264
    #define _SYS_linkat   265
    #define _SYS_symlinkat    266
    #define _SYS_readlinkat   267
    #define _SYS_fchmodat   268
    #define _SYS_faccessat    269
    #define _SYS_pselect6   270
    #define _SYS_ppoll    271
    #define _SYS_unshare    272
    #define _SYS_set_robust_list  273
    #define _SYS_get_robust_list  274
    #define _SYS_splice   275
    #define _SYS_tee    276
    #define _SYS_sync_file_range  277
    #define _SYS_vmsplice   278
    #define _SYS_move_pages   279
    #define _SYS_utimensat    280
  */

  // int openat(int dirfd, const char *pathname, int flags, ...){
  //   va_list ap;
  //   va_start(ap, flags);
  //   mode_t mode  = va_arg(ap, mode_t);
  // //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().openat(dirfd, pathname, flags, mode);
  // }

  // int openat(int dirfd, const char *pathname, int flags, mode_t mode){
  //   return syscalls::getInstance().openat(dirfd, pathname, flags, mode);
  // }

  int mkdirat(int dirfd, const char* pathname, mode_t mode) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().mkdirat(dirfd, pathname, mode);
  }

  // int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev){
  //   return syscalls::getInstance().mknodat(dirfd, pathname, mode, dev);
  // }

  int fchownat(int dirfd, const char* path, uid_t owner, gid_t group, int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fchownat(dirfd, path, owner, group, flags);
  }

  int futimesat(int dirfd, const char* path, const struct timeval times[2]) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().futimesat(dirfd, path, times);
  }

  int unlinkat(int dirfd, const char* pathname, int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().unlinkat(dirfd, pathname, flags);
  }

  int renameat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().renameat(olddirfd, oldpath, newdirfd, newpath);
  }

  int linkat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().linkat(olddirfd, oldpath, newdirfd, newpath, flags);
  }

  int symlinkat(const char* oldpath, int newdirfd, const char* newpath) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().symlinkat(oldpath, newdirfd, newpath);
  }

  ssize_t readlinkat(int dirfd, const char* path, char* buf, size_t bufsiz) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().readlinkat(dirfd, path, buf, bufsiz);
  }

  int fchmodat(int dirfd, const char* path, mode_t mode, int flags) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().fchmodat(dirfd, path, mode, flags);
  }

  int faccessat(int dirfd, const char* path, int mode, int flags) {

  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().faccessat(dirfd, path, mode, flags);
  }

  int pselect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
	      const struct timespec* timeout, const sigset_t* sigmask) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
  }

  int ppoll(struct pollfd* fds, nfds_t nfds, const struct timespec* timeout,
	    const sigset_t* sigmask) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().ppoll(fds, nfds, timeout, sigmask);
  }

  int unshare(int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().unshare(flags);
  }

  // No libc wrapper
  // long get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr){
  //   return syscalls::getInstance().get_robust_list(pid, head_ptr, len_ptr);
  // }

  // No libc wrapper
  // long set_robust_list(struct robust_list_head *head, size_t len){
  // //fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().set_robust_list(head, len);
  // }

  ssize_t splice(int fd_in, __off64_t* off_in, int fd_out, __off64_t* off_out, size_t len,
		 unsigned int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().splice(fd_in, off_in, fd_out, off_out, len, flags);
  }

  ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().tee(fd_in, fd_out, len, flags);
  }

  int sync_file_range(int fd, __off64_t offset, __off64_t nbytes, unsigned int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().sync_file_range(fd, offset, nbytes, flags);
  }

  ssize_t vmsplice(int fd, const struct iovec* iov, size_t nr_segs, unsigned int flags) {
  //fprintf(stderr, " in doubletake at %d\n", __LINE__);
    return syscalls::getInstance().vmsplice(fd, iov, nr_segs, flags);
  }

  // No libc wrapper
  // long move_pages(pid_t pid, unsigned long nr_pages, const void **address, const int *nodes, int
  // *status, int flags){
  ////fprintf(stderr, " in doubletake at %d\n", __LINE__);
  //   return syscalls::getInstance().move_pages(pid, nr_pages, address, nodes, status, flags);
  // }

#endif
};
