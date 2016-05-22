#if !defined(DOUBLETAKE_SYSCALLS_H)
#define DOUBLETAKE_SYSCALLS_H

/*
 * @file   syscalls.h
 * @brief  The main entry for system calls etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <dirent.h>
#include <mqueue.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>

#include <new>

#include "doubletake.hh"
#include "log.hh"
#include "fops.hh"
#include "real.hh"
#include "sysrecord.hh"
#include "threadstruct.hh"

class syscalls {
public:
  syscalls() : _sysrecord(), _fops() {
  }

  /// @brief Initialize the system.
  void initialize() { _fops.initialize(); }

  // Currently, epochBegin() will call xrun::epochBegin().
  void epochBegin();
  void epochEnd();

  // Simply commit specified memory block
  void atomicCommit(void* addr, size_t size);

  // Called by xrun::epochEnd when there is no overflow.
  // in the end of checking when an epoch ends.
  // Now, only one thread is active.
  void epochEndWell() {
    // Cleanup all closed files so that we don't have to 
		// update those files and directories. 
    _fops.cleanClosedFiles();

    // Try to updating those information for all opened files.
    // Currently, we don't know whether a file is closed or not.
    // Only the thread to close a file will know that.
    // It is wasting to update information about a file that has been closed.
    // But we can avoid start all threads to integrate those stopped files and stop
    // them again.
    _fops.updateOpenedFiles();
  }

  // Prepare rollback for system calls
  void prepareRollback() {
		fprintf(stderr, "syscalls: prepareRollback at thread\n");
    // Handle all opened files
    _fops.prepareRollback();
  }

  void checkOverflowBeforehand(void* start, size_t size) {
    // Make those pages writable, otherwise, read may fail
    makeWritable(start, size);
    /*
        PRINF("CHECK whether system call can overflow\n");
        if(_memory.checkOverflowBeforehand(start, size)) {
          PRINF("System call can overflow, start %p size %x\n", start, size);
          _memory.printCallsite();
          assert(0);
        }
        PRINF("CHECK whether system call can overflow done!!!!!\n");
    */
  }

  void makeWritable(void* buf, int count) {
    //#ifdef PROTECT_MEMORY
    char* start = (char*)buf;
    long pages = count / xdefines::PageSize;

    if(pages >= 1) {
      // Trying to read on those pages, thus there won't be a segmentation fault in
      // the system call.
      for(long i = 0; i < pages; i++) {
        start[i * xdefines::PageSize] = '\0';
      }
    }
    start[count - 1] = '\0';
    //#endif
    return;
  }

  ssize_t read(int fd, void* buf, size_t count) {
    ssize_t ret;

    // Make those pages writable, otherwise, read may fail
    makeWritable(buf, count);

    // PRINF("read on fd %d\n", fd);
    // Check whether this fd is not a socketid.
    if(_fops.checkPermission(fd)) {
      ret = Real::read(fd, buf, count);
    } else {
      //      PRINF("Reading special file\n");
      ret = Real::read(fd, buf, count);
      epochEnd();
      epochBegin();
    }

    return ret;
  }

  ssize_t write(int fd, const void* buf, size_t count) {
    ssize_t ret;

    // writes always need to end an epoch, otherwise we could end up
    // with double-writes if we rollback.
    epochEnd();
    ret = Real::write(fd, buf, count);
    epochBegin();

    return ret;
  }

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
  // We may record the address in order to replay
  // Tongping
  void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset) {
    void* ret = NULL;
    if(!doubletake::inRollback) {
      // We only record these mmap requests.
      ret = Real::mmap(start, length, prot, flags, fd, offset);
      //      WARN("in execution, ret %p length %lx\n", ret, length);
      _sysrecord.recordMmapOps(ret);
    } else {
      _sysrecord.getMmapOps(&ret);
    }
#if 0 // Used to test epochBegin
    PRINT("in the end of mmap, ret %p length %lx\n", ret, length);
    epochEnd();
    ret = Real::mmap(start, length, prot, flags, fd, offset);
    epochBegin();
    PRINF("in the end of mmap, ret %p length %lx\n", ret, length);
#endif
    return ret;
  }

  // We may record fd in order replay that.
  int open(const char* pathname, int flags) { return open(pathname, flags, 0); }

  // Tongping
  int open(const char* pathname, int flags, mode_t mode) {
    int ret;

    // In the rollback phase, we only call
    if(!doubletake::inRollback) {
      ret = Real::open(pathname, flags, mode);
      // Save current fd, pass NULL since it is not a file stream
      _fops.saveFd(ret, NULL);
    } else {
      ret = _fops.getFdAtOpen();
    }
    return ret;
  }

  int close(int fd) {
    int ret = 0;

    if(_fops.isNormalFile(fd)) {
      // In the rollback phase, we only call
      if(!doubletake::inRollback) {
       	ret = _fops.closeFile(fd, NULL);
      } else {
        ret = _fops.getClose(fd);
      }
    }
    else {
      // selfmap::getInstance().printCallStack(NULL, NULL, true);
      epochEnd();
      ret = Real::close(fd);
      epochBegin();
    }
    return ret;
  }

  DIR* opendir(const char* name) {
    DIR* ret;

    // In the normal phase
    if(!doubletake::inRollback) {
      ret = Real::opendir(name);
      // Save current fd, pass NULL since it is not a file stream
      _fops.saveDir(ret);
    } else {
      ret = _fops.getDirAtOpen();
    }
    PRINF("(((((((((((((((OPEN dir %s)))))))))\n", name);

    return ret;
  }

  int closedir(DIR* dir) {
    int ret;

    if(!doubletake::inRollback) {
      ret = _fops.closeDir(dir);
    } else {
			// We only need to 
      ret = _fops.getCloseDir(dir);
    }

    return ret;
  }

	// Use the default mechanism for seekdir.
	void seekdir(DIR *dir, long pos) {
    epochEnd();
    Real::seekdir(dir, pos);
    epochBegin();
  }

  void rewinddir(DIR *dir) {
    epochEnd();
    Real::rewinddir(dir);
    epochBegin();
  }

  FILE* fopen(const char* filename, const char* modes) {
    FILE* ret = NULL;

    if(!doubletake::inRollback) {
      ret = Real::fopen(filename, modes);
      if(ret != NULL) {
#if 0
      unsigned long * ptr = (unsigned long *) ret;
      unsigned long i;
      for(i = 0; i < xdefines::FOPEN_ALLOC_SIZE/sizeof(unsigned long); i++) {
        fprintf(stderr, "i %ld addr %p content %lx\n", i, ptr, *ptr);
        ptr++;
      }
      fprintf(stderr, "before inserting\n");
#endif
        // Save current fd
	      _fops.saveFopen(ret);
				//printf("fopen fp %p fileno %d\n", ret, ret->_fileno);
#if 0
      fprintf(stderr, "after inserting\n");
      for(i = 0; i < xdefines::FOPEN_ALLOC_SIZE/sizeof(unsigned long); i++) {
        fprintf(stderr, "i %ld addr %p content %lx\n", i, ptr, *ptr);
        ptr++;
      }
#endif
      } 
    } else {
      // rollback phase
      ret = _fops.getFopen();
    }

    return ret;
  }

  FILE* fopen64(const char* filename, const char* modes) {
    FILE* ret;

    if(!doubletake::inRollback) {
      ret = Real::fopen64(filename, modes);
      if(ret != NULL) {
        // Save current fd
        _fops.saveFopen(ret);
				//printf("fopen64 fp %p fileno %d\n", ret, ret->_fileno);
       // selfmap::getInstance().printCallStack();
      } else {
        _fops.saveFd(-1, NULL);
      }
    } else {
      // rollback phase
      ret = _fops.getFopen();
    }

    return ret;
  }

  // Delay actual fclose instead.
  int fclose(FILE* fp) {
    int ret;

    assert(fp != NULL);

    // flush the result, required by the posix standard
    fflush(fp);

		if(!doubletake::inRollback) {
//      selfmap::getInstance().printCallStack();
			//printf("fclose fp %p fileno %d\n", fp, fp->_fileno);
      ret = _fops.closeFile(fp->_fileno, fp);
    } else {
      // We only need to 
      ret = _fops.getCloseFile(fp->_fileno, fp);
    }

    return ret;
  }

  int stat(const char* path, struct stat* buf) {
    int ret;
    epochEnd();
    ret = Real::stat(path, buf);
    epochBegin();
    return ret;
  }

  int fstat(int filedes, struct stat* buf) {
    int ret;
    epochEnd();
    ret = Real::fstat(filedes, buf);
    epochBegin();
    return ret;
  }

  int lstat(const char* path, struct stat* buf) {
    int ret;
    epochEnd();
    ret = Real::lstat(path, buf);
    epochBegin();
    return ret;
  }

  int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    int ret;
    epochEnd();
    ret = Real::poll(fds, nfds, timeout);
    epochBegin();
    return ret;
  }

  off_t lseek(int filedes, off_t offset, int whence) {
    off_t ret;
		if(whence == SEEK_CUR && offset == 0) {
			// For performance reason, it is ok to only check the offset. 
    	ret = Real::lseek(filedes, offset, whence);
		}
		else {
    	epochEnd();
    	ret = Real::lseek(filedes, offset, whence);
    	epochBegin();
		}
    return ret;
  }

  int mprotect(void* addr, size_t len, int prot) {
    int ret;

    // FIXME: since pthread_create will call mprotect, we don't
    // want to introduce some unnecessary checking here.
    epochEnd();
    ret = Real::mprotect(addr, len, prot);
    epochBegin();
    return ret;
  }

  int munmap(void* start, size_t length) {
    int ret = 0;

    if(!doubletake::inRollback) {
      _sysrecord.recordMunmapOps(start, length);
    } else {
      _sysrecord.getMunmapOps();
    }
    return ret;
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
    int ret;
    epochEnd();
    ret = Real::brk(end_data_segment);
    epochBegin();
    return ret;
  }

  int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
    int ret;
    epochEnd();
    ret = Real::sigaction(signum, act, oldact);
    epochBegin();
    return ret;
  }

  int sigprocmask(int how, const sigset_t* set, sigset_t* oldset) {
    int ret;
    epochEnd();
    ret = Real::sigprocmask(how, set, oldset);
    epochBegin();
    return ret;
  }

  int sigreturn(unsigned long __unused) {
    int ret;
    epochEnd();
    ret = Real::sigreturn((struct sigcontext*)__unused);
    epochBegin();
    return ret;
  }

  ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    ssize_t ret;

    checkOverflowBeforehand(buf, count);
    if(_fops.checkPermission(fd)) {
      ret = Real::pread(fd, buf, count, offset);
    } else {
      epochEnd();
      ret = Real::pread(fd, buf, count, offset);
      epochBegin();
    }
    return ret;
  }

  ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    ssize_t ret;

    if(_fops.checkPermission(fd)) {
      ret = Real::pwrite(fd, buf, count, offset);
    } else {
      epochEnd();
      ret = Real::pwrite(fd, buf, count, offset);
      epochBegin();
    }
    return ret;
  }

  ssize_t readv(int fd, const struct iovec* vector, int count) {
    ssize_t ret;

    for (int i = 0; i < count; i++) {
      checkOverflowBeforehand((vector+i)->iov_base, (vector+i)->iov_len);
    }

    if(_fops.checkPermission(fd)) {
      ret = Real::readv(fd, vector, count);
    } else {
      // No need to call aotmicBegin() since this system call
      // won't cause overflow.
      ret = Real::readv(fd, vector, count);
      epochEnd();

      for(int i = 0; i < count; i++) {
        atomicCommit((vector+i)->iov_base, (vector+i)->iov_len);
      }
      epochBegin();
    }
    return ret;
  }

  ssize_t writev(int fd, const struct iovec* vector, int count) {
    ssize_t ret;

    // Check whether this fd is not a socketid.
    if(_fops.checkPermission(fd)) {
      ret = Real::writev(fd, vector, count);
    } else {
      epochEnd();
      ret = Real::writev(fd, vector, count);
      epochBegin();
    }
    return ret;
  }

  /*
    int access(const char *pathname, int mode){
      int ret;
      epochEnd();
      ret = Real::access(pathname, mode);
      epochBegin();
      return ret;
    }
  */

  int pipe(int filedes[2]) {
    int ret;
    epochEnd();
    ret = Real::pipe(filedes);
    epochBegin();
    return ret;
  }

  int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
             struct timeval* timeout) {
    int ret;
    epochEnd();
    ret = Real::select(nfds, readfds, writefds, exceptfds, timeout);
    epochBegin();
    return ret;
  }

  void* mremap(void* old_address, size_t old_size, size_t new_size, int flags) {
    void* ret;
    epochEnd();
    ret = Real::mremap(old_address, old_size, new_size, flags);
    epochBegin();
    return ret;
  }

  int msync(void* start, size_t length, int flags) {
    int ret;
    epochEnd();
    ret = Real::msync(start, length, flags);
    epochBegin();
    return ret;
  }

  int mincore(void* start, size_t length, unsigned char* vec) {
    int ret;
    epochEnd();
    ret = Real::mincore(start, length, vec);
    epochBegin();
    return ret;
  }

  int madvise(void* start, size_t length, int advice) {
    int ret;
    epochEnd();
    ret = Real::madvise(start, length, advice);
    epochBegin();
    return ret;
  }

  // Tongping, FIXME, we should record this.
  int shmget(key_t key, size_t size, int shmflg) {
    int ret;
    epochEnd();

    ret = Real::shmget(key, size, shmflg);
    epochBegin();
    return ret;
  }

  // Tongping, FIXME, we should record this.
  void* shmat(int shmid, const void* shmaddr, int shmflg) {
    void* ret;
    epochEnd();
    ret = Real::shmat(shmid, shmaddr, shmflg);
    epochBegin();
    return ret;
  }

  int shmctl(int shmid, int cmd, struct shmid_ds* buf) {
    int ret;
    epochEnd();

    ret = Real::shmctl(shmid, cmd, buf);
    epochBegin();
    return ret;
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

  // RECORD this, Tongping
  int dup(int oldfd) {
    int ret = 0;

    if(_fops.isNormalFile(oldfd)) {
      if(!doubletake::inRollback) {
        ret = Real::dup(oldfd);
        // Save current fd, pass NULL since it is not a file stream
        _fops.saveDupFd(oldfd, ret);
      } else {
        ret = _fops.getDupFd();
      }
    }
    else {
      epochEnd();
      ret = Real::dup(oldfd);
      epochBegin();
    }
    return ret;
  }

  // RECORD this, Tongping
  int dup2(int oldfd, int newfd) {
    int ret;

    if(_fops.isNormalFile(newfd)) {
      if(!doubletake::inRollback) {
        ret = Real::dup2(oldfd, newfd);
        // Save current fd, pass NULL since it is not a file stream
        _fops.saveDupFd(oldfd, ret);
      } else {
        ret = _fops.getFdAtOpen();
      }
    }
    else {
      // newfd is an opened file descriptor,
      // We have to stop the phase.
      epochEnd();
      ret = Real::dup2(oldfd, newfd);
      epochBegin();
    }

    return ret;
  }

  int pause() {
    int ret;
    epochEnd();
    ret = Real::pause();
    epochBegin();
    return ret;
  }

  int nanosleep(const struct timespec* req, struct timespec* rem) {
    int ret;
    epochEnd();
    ret = Real::nanosleep(req, rem);
    epochBegin();
    return ret;
  }

  int getitimer(int which, struct itimerval* value) {
    int ret;
    epochEnd();

    ret = Real::getitimer(which, value);
    epochBegin();
    return ret;
  }

  unsigned int alarm(unsigned int seconds) {
    int ret;
    epochEnd();

    ret = Real::alarm(seconds);
    epochBegin();
    return ret;
  }

  int setitimer(int which, const struct itimerval* value, struct itimerval* ovalue) {
    int ret;
    epochEnd();

    ret = Real::setitimer(which, value, ovalue);
    if(ovalue != NULL) {
      atomicCommit(ovalue, sizeof(struct itimerval));
    }
    epochBegin();
    return ret;
  }

  // pid_t getpid()

  ssize_t sendfile(int out_fd, int in_fd, off_t* offset, size_t count) {
    ssize_t ret;
    epochEnd();

    ret = Real::sendfile(out_fd, in_fd, offset, count);
    epochBegin();
    return ret;
  }

  int socket(int domain, int type, int protocol) {
    int ret;
    epochEnd();
    ret = Real::socket(domain, type, protocol);
    epochBegin();
    return ret;
  }

  int connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen) {
    int ret;
    epochEnd();
    ret = Real::connect(sockfd, serv_addr, addrlen);
    epochBegin();
    return ret;
  }

  int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int ret;
    epochEnd();

    ret = Real::accept(sockfd, addr, addrlen);
    // Check whether this buffer is overflow something.
    if(ret > 0) {
      checkOverflowBeforehand(addr, ret);
      atomicCommit(addr, ret);
    }
    epochBegin();
    return ret;
  }

  ssize_t sendto(int s, const void* buf, size_t len, int flags, const struct sockaddr* to,
                 socklen_t tolen) {
    ssize_t ret;
    epochEnd();
    ret = Real::sendto(s, buf, len, flags, to, tolen);
    epochBegin();
    return ret;
  }

  ssize_t recvfrom(int s, void* buf, size_t len, int flags, struct sockaddr* from,
                   socklen_t* fromlen) {
    ssize_t ret;
    epochEnd();

    checkOverflowBeforehand(from, len);
    ret = Real::recvfrom(s, buf, len, flags, from, fromlen);
    if(ret > 0) {
      atomicCommit(from, ret);
    }
    epochBegin();
    return ret;
  }

  ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {
    ssize_t ret;
    epochEnd();

    ret = Real::sendmsg(s, msg, flags);
    epochBegin();
    return ret;
  }

  ssize_t recvmsg(int s, struct msghdr* msg, int flags) {
    ssize_t ret;
    epochEnd();

    ret = Real::recvmsg(s, msg, flags);
    epochBegin();
    return ret;
  }

  int shutdown(int sockfd, int how){
    int ret;
    epochEnd();
    ret = Real::shutdown(sockfd, how);
    epochBegin();
    return ret;
  }

  int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen) {
    int ret;
    epochEnd();

    ret = Real::bind(sockfd, my_addr, addrlen);
    epochBegin();
    return ret;
  }

  int listen(int sockfd, int backlog) {
    int ret;
    epochEnd();

    ret = Real::listen(sockfd, backlog);
    epochBegin();
    return ret;
  }

  int getsockname(int s, struct sockaddr* name, socklen_t* namelen) {
    int ret;
    epochEnd();

    ret = Real::getsockname(s, name, namelen);
    if(ret > 0) {
      checkOverflowBeforehand(name, ret);
      atomicCommit(name, ret);
    }
    epochBegin();
    return ret;
  }

  int getpeername(int s, struct sockaddr* name, socklen_t* namelen) {
    int ret;
    epochEnd();

    ret = Real::getpeername(s, name, namelen);
    if(ret > 0) {
      checkOverflowBeforehand(name, ret);
      atomicCommit(name, ret);
    }
    epochBegin();
    return ret;
  }

  int socketpair(int d, int type, int protocol, int sv[2]) {

    int ret;
    epochEnd();
    ret = Real::socketpair(d, type, protocol, sv);
    if(ret == 0) {
      atomicCommit(&sv[0], sizeof(int));
      atomicCommit(&sv[1], sizeof(int));
    }
    epochBegin();
    return ret;
  }

  int setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {

    int ret;
    epochEnd();
    ret = Real::setsockopt(s, level, optname, optval, optlen);
    epochBegin();
    return ret;
  }

  int getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen) {

    int ret;
    epochEnd();
    checkOverflowBeforehand(optval, *optlen);
    ret = Real::getsockopt(s, level, optname, optval, optlen);
    if(ret) {
      atomicCommit(optval, *optlen);
    }
    epochBegin();
    return ret;
  }

  //  int clone(int (*fn)(void *), void *child_stack,

  //  pid_t fork(){

  //  pid_t vfork(){
  // FIXME

  int execve(const char* filename, char* const argv[], char* const envp[]) {

    int ret;
    epochEnd();
    ret = Real::execve(filename, argv, envp);
    epochBegin();
    return ret;
  }

  //  void exit(int status){

  pid_t wait4(pid_t pid, void* status, int options, struct rusage* rusage) {

    pid_t ret;
    epochEnd();
    ret = Real::wait4(pid, status, options, rusage);
    epochBegin();
    return ret;
  }

  int kill(pid_t pid, int sig) {
    // FIXME
    int ret;
    epochEnd();
    ret = Real::kill(pid, sig);
    epochBegin();
    return ret;
  }

  int uname(struct utsname* buf) {

    int ret;
    epochEnd();
    ret = Real::uname(buf);
    epochBegin();
    return ret;
  }

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

  int semget(key_t key, int nsems, int semflg) {
    int ret;
    epochEnd();

    ret = Real::semget(key, nsems, semflg);
    epochBegin();
    return ret;
  }

  int semop(int semid, struct sembuf* sops, size_t nsops) {

    int ret;
    epochEnd();
    ret = Real::semop(semid, sops, nsops);
    epochBegin();
    return ret;
  }

  int semctl(int semid, int semnum, int cmd, ...) {
    int ret;
    epochEnd();
    ret = Real::semctl(semid, semnum, cmd);
    epochBegin();
    return ret;
  }

  int fcntl(int fd, int cmd, long arg) {
    int ret;

    switch(cmd) {
    case F_DUPFD: {
      // In the rollback phase, we only call
      if(doubletake::inRollback) {
        ret = _fops.getFdAtOpen();
      } else {
        ret = Real::fcntl(fd, cmd, arg);
        // Save current fd, pass NULL since it is not a file stream
        _fops.saveFd(ret, NULL);
      }
      break;
    }
   
		/* For the following commands, there is no need to do anything */ 
		case F_GETFL:
    /* Fall through */
    case F_GETLK:
		case F_GETOWN:
		case F_GETOWN_EX:
		case F_GETSIG:
		case F_GETPIPE_SZ:
		{
      ret = Real::fcntl(fd, cmd, arg);
			break;
		}

    case F_SETFD:
		{
			if(arg == FD_CLOEXEC) {
				ret = Real::fcntl(fd, cmd, arg);
      	break;
			}
			// else, fall through
		}
    /* Fall through */
    default: {
      epochEnd();
      ret = Real::fcntl(fd, cmd, arg);
      epochBegin();
      break;
    }
    }
    return ret;
  }

  int flock(int fd, int operation) {

    int ret;
    epochEnd();
    ret = Real::flock(fd, operation);
    epochBegin();
    return ret;
  }

  int fsync(int fd) {

    int ret;
    epochEnd();
    ret = Real::fsync(fd);
    epochBegin();
    return ret;
  }

  int fdatasync(int fd) {

    int ret;
    epochEnd();
    ret = Real::fdatasync(fd);
    epochBegin();
    return ret;
  }

  int truncate(const char* path, off_t length) {

    int ret;
    epochEnd();
    ret = Real::truncate(path, length);
    epochBegin();
    return ret;
  }

  int ftruncate(int fd, off_t length) {
    int ret;
    epochEnd();

    ret = Real::ftruncate(fd, length);
    epochBegin();
    return ret;
  }

  // getdents is not defined
  /*
  int getdents(unsigned int fd, struct dirent *dirp, unsigned int count){
    int ret;
    epochEnd();

    ret = Real::getdents(fd, dirp, count);
    epochBegin();
    return ret;
  }
  */

  char* getcwd(char* buf, size_t size) {
    char* ret;
    epochEnd();
    checkOverflowBeforehand(buf, size);
    ret = Real::getcwd(buf, size);
    atomicCommit(buf, size);
    epochBegin();
    return ret;
  }

  int chdir(const char* path) {
    int ret;
    epochEnd();

    ret = Real::chdir(path);
    epochBegin();
    return ret;
  }

  int fchdir(int fd) {
    int ret;
    epochEnd();

    ret = Real::fchdir(fd);
    epochBegin();
    return ret;
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
    int ret;
    epochEnd();
    ret = Real::rename(oldpath, newpath);
    epochBegin();
    return ret;
  }

  int mkdir(const char* pathname, mode_t mode) {
    int ret;
    epochEnd();
    ret = Real::mkdir(pathname, mode);
    epochBegin();
    return ret;
  }

  int rmdir(const char* pathname) {

    int ret;
    epochEnd();
    ret = Real::rmdir(pathname);
    epochBegin();
    return ret;
  }

  int creat(const char* pathname, mode_t mode) {
    int ret;
    epochEnd();

    ret = Real::creat(pathname, mode);
    epochBegin();
    return ret;
  }

  int link(const char* oldpath, const char* newpath) {

    int ret;
    epochEnd();
    ret = Real::link(oldpath, newpath);
    epochBegin();
    return ret;
  }

  int unlink(const char* pathname) {

    int ret;
    epochEnd();
    ret = Real::unlink(pathname);
    epochBegin();
    return ret;
  }

  int symlink(const char* oldpath, const char* newpath) {

    int ret;
    epochEnd();
    ret = Real::symlink(oldpath, newpath);
    epochBegin();
    return ret;
  }

  ssize_t readlink(const char* path, char* buf, size_t bufsize) {

    ssize_t ret;
    epochEnd();
    ret = Real::readlink(path, buf, bufsize);
    if(bufsize) {
      atomicCommit(buf, bufsize);
    }
    epochBegin();
    return ret;
  }

  int chmod(const char* path, mode_t mode) {
    int ret;
    epochEnd();

    ret = Real::chmod(path, mode);
    epochBegin();
    return ret;
  }

  int fchmod(int fildes, mode_t mode) {

    int ret;
    epochEnd();
    ret = Real::fchmod(fildes, mode);
    epochBegin();
    return ret;
  }

  int chown(const char* path, uid_t owner, gid_t group) {

    int ret;
    epochEnd();
    ret = Real::chown(path, owner, group);
    epochBegin();
    return ret;
  }

  int fchown(int fd, uid_t owner, gid_t group) {
    int ret;
    epochEnd();
    ret = Real::fchown(fd, owner, group);
    epochBegin();
    return ret;
  }

  int lchown(const char* path, uid_t owner, gid_t group) {

    int ret;
    epochEnd();
    ret = Real::lchown(path, owner, group);
    epochBegin();
    return ret;
  }

  mode_t umask(mode_t mask) {

    mode_t ret;
    epochEnd();
    ret = Real::umask(mask);
    epochBegin();
    return ret;
  }

  // We can record this also. Tongping
  int gettimeofday(struct timeval* tv, struct timezone* tz) {
    int ret = 0;
    if(!doubletake::inRollback) {
      ret = Real::gettimeofday(tv, tz);
      // Add this to the record list.
      _sysrecord.recordGettimeofdayOps(ret, tv, tz);
    } else {
      if(!_sysrecord.getGettimeofdayOps(&ret, tv, tz)) {
        assert(0);
      }
    }
    return ret;
  }

  int getrlimit(int resource, struct rlimit* rlim) {
    int ret;
    epochEnd();

    ret = Real::getrlimit(resource, rlim);
    epochBegin();
    return ret;
  }

  int getrusage(int who, struct rusage* usage) {
    int ret;
    epochEnd();

    ret = Real::getrusage(who, usage);
    epochBegin();
    return ret;
  }

  int sysinfo(struct sysinfo* info) {
    int ret;
    epochEnd();
    ret = Real::sysinfo(info);
    epochBegin();
    return ret;
  }

  clock_t times(struct tms* buf) {
    clock_t ret;

    if(!doubletake::inRollback) {
      ret = Real::times(buf);
      // Add this to the record list.
      _sysrecord.recordTimesOps(ret, buf);
    } else {
      if(!_sysrecord.getTimesOps(&ret, buf)) {
        assert(0);
      }
    }
    return ret;
  }

  //  long ptrace(enum __ptrace_request request, pid_t pid,
  //              void *addr, void *data){
  // FIXME

  uid_t getuid() {
    uid_t ret;
    epochEnd();
    ret = Real::getuid();
    epochBegin();
    return ret;
  }

  void vsyslog(int pri, const char* fmt, va_list args) {
    epochEnd();
    Real::vsyslog(pri, fmt, args);
    epochBegin();
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

  gid_t getgid() {

    gid_t ret;
    epochEnd();
    ret = Real::getgid();
    epochBegin();
    return ret;
  }

  int setuid(uid_t uid) {
    int ret;
    epochEnd();

    ret = Real::setuid(uid);
    epochBegin();
    return ret;
  }

  int setgid(gid_t gid) {
    int ret;
    epochEnd();

    ret = Real::setgid(gid);
    epochBegin();
    return ret;
  }

  uid_t geteuid() {
    uid_t ret;
    epochEnd();

    ret = Real::geteuid();
    epochBegin();
    return ret;
  }

  gid_t getegid() {
    gid_t ret;
    epochEnd();

    ret = Real::getegid();
    epochBegin();
    return ret;
  }

  int setpgid(pid_t pid, pid_t pgid) {
    int ret;
    epochEnd();

    ret = Real::setpgid(pid, pgid);
    epochBegin();
    return ret;
  }

  pid_t getppid() {
    pid_t ret;
    epochEnd();

    ret = Real::getppid();
    epochBegin();
    return ret;
  }

  pid_t getpgrp() {
    pid_t ret;
    epochEnd();

    ret = Real::getpgrp();
    epochBegin();
    return ret;
  }

  pid_t setsid() {
    pid_t ret;
    epochEnd();

    ret = Real::setsid();
    epochBegin();
    return ret;
  }

  int setreuid(uid_t ruid, uid_t euid) {
    int ret;
    epochEnd();

    ret = Real::setreuid(ruid, euid);
    epochBegin();
    return ret;
  }

  int setregid(gid_t rgid, gid_t egid) {
    int ret;
    epochEnd();

    ret = Real::setregid(rgid, egid);
    epochBegin();
    return ret;
  }

  int getgroups(int size, gid_t list[]) {
    int ret;
    epochEnd();

    ret = Real::setgroups(size, list);
    epochBegin();
    return ret;
  }

  int setgroups(size_t size, const gid_t* list) {
    int ret;
    epochEnd();

    ret = Real::setgroups(size, list);
    epochBegin();
    return ret;
  }

  int setresuid(uid_t ruid, uid_t euid, uid_t suid) {
    int ret;
    epochEnd();

    ret = Real::setresuid(ruid, euid, suid);
    epochBegin();
    return ret;
  }

  int getresuid(uid_t* ruid, uid_t* euid, uid_t* suid) {
    int ret;
    epochEnd();

    ret = Real::getresuid(ruid, euid, suid);
    epochBegin();
    return ret;
  }

  int setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
    int ret;
    epochEnd();

    ret = Real::setresgid(rgid, egid, sgid);
    epochBegin();
    return ret;
  }

  int getresgid(gid_t* rgid, gid_t* egid, gid_t* sgid) {
    int ret;
    epochEnd();

    ret = Real::getresgid(rgid, egid, sgid);
    epochBegin();
    return ret;
  }

  pid_t getpgid(pid_t pid) {
    pid_t ret;
    epochEnd();

    ret = Real::getpgid(pid);
    epochBegin();
    return ret;
  }

  int setfsuid(uid_t fsuid) {
    int ret;
    epochEnd();

    ret = Real::setfsuid(fsuid);
    epochBegin();
    return ret;
  }

  int setfsgid(uid_t fsgid) {
    int ret;
    epochEnd();

    ret = Real::setfsgid(fsgid);
    epochBegin();
    return ret;
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

  pid_t getsid(pid_t pid) {
    pid_t ret;
    epochEnd();
    ret = Real::getsid(pid);
    epochBegin();
    return ret;
  }

  int sigpending(sigset_t* set) {
    int ret;
    epochEnd();

    ret = Real::sigpending(set);
    epochBegin();
    return ret;
  }

  int sigtimedwait(const sigset_t* set, siginfo_t* info, const struct timespec* timeout) {
    int ret;
    epochEnd();

    ret = Real::sigtimedwait(set, info, timeout);
    epochBegin();
    return ret;
  }

  int sigsuspend(const sigset_t* mask) {
    int ret;
    epochEnd();

    ret = Real::sigsuspend(mask);
    epochBegin();
    return ret;
  }

  int sigaltstack(const stack_t* ss, stack_t* oss) {
    int ret;
    epochEnd();

    ret = Real::sigaltstack(ss, oss);
    epochBegin();
    return ret;
  }

  int utime(const char* filename, const struct utimbuf* buf) {
    int ret;
    epochEnd();

    ret = Real::utime(filename, buf);
    epochBegin();
    return ret;
  }

  int mknod(const char* pathname, mode_t mode, dev_t dev) {
    int ret;
    epochEnd();

    ret = Real::mknod(pathname, mode, dev);
    epochBegin();
    return ret;
  }

  // uselib is not defined
  /*
  int uselib(const char *library){
    int ret;
    epochEnd();
    ret = Real::uselib(library);
    epochBegin();
    return ret;
  }
  */

  int personality(unsigned long persona) {

    int ret;
    epochEnd();
    ret = Real::personality(persona);
    epochBegin();
    return ret;
  }

  int ustat(dev_t dev, struct ustat* ubuf) {

    int ret;
    epochEnd();
    ret = Real::ustat(dev, ubuf);
    epochBegin();
    return ret;
  }

  int statfs(const char* path, struct statfs* buf) {

    int ret;
    epochEnd();
    ret = Real::statfs(path, buf);
    epochBegin();
    return ret;
  }

  int fstatfs(int fd, struct statfs* buf) {

    int ret;
    epochEnd();
    ret = Real::fstatfs(fd, buf);
    epochBegin();
    return ret;
  }

  // sysfs isn't defined anywhere
  /*
  int sysfs(int option, unsigned int fs_index, char *buf){
    int ret;
    epochEnd();
    ret = Real::sysfs(option, fs_index, buf);
    epochBegin();
    return ret;
  }*/

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

  int getpriority(int which, int who) {
    int ret;
    epochEnd();

    ret = Real::getpriority(which, who);
    epochBegin();
    return ret;
  }

  int setpriority(__priority_which_t which, id_t who, int prio) {
    int ret;
    epochEnd();

    ret = Real::setpriority(which, who, prio);
    epochBegin();
    return ret;
  }

  int sched_setparam(pid_t pid, const struct sched_param* param) {

    int ret;
    epochEnd();
    ret = Real::sched_setparam(pid, param);
    epochBegin();
    return ret;
  }

  int sched_getparam(pid_t pid, struct sched_param* param) {

    int ret;
    epochEnd();
    ret = Real::sched_getparam(pid, param);
    epochBegin();
    return ret;
  }

  int sched_setscheduler(pid_t pid, int policy, const struct sched_param* param) {
    int ret;
    epochEnd();
    ret = Real::sched_setscheduler(pid, policy, param);
    epochBegin();
    return ret;
  }

  int sched_getscheduler(pid_t pid) {
    int ret;
    epochEnd();
    ret = Real::sched_getscheduler(pid);
    epochBegin();
    return ret;
  }

  int sched_get_priority_max(int policy) {
    int ret;
    epochEnd();
    ret = Real::sched_get_priority_max(policy);
    epochBegin();
    return ret;
  }

  int sched_get_priority_min(int policy) {
    int ret;
    epochEnd();
    ret = Real::sched_get_priority_min(policy);
    epochBegin();
    return ret;
  }

  int sched_rr_get_interval(pid_t pid, struct timespec* tp) {
    int ret;
    epochEnd();
    ret = Real::sched_rr_get_interval(pid, tp);
    epochBegin();
    return ret;
  }

  int mlock(const void* addr, size_t len) {
    int ret;
    epochEnd();
    ret = Real::mlock(addr, len);
    epochBegin();
    return ret;
  }

  int munlock(const void* addr, size_t len) {
    int ret;
    epochEnd();

    ret = Real::munlock(addr, len);
    epochBegin();
    return ret;
  }

  int mlockall(int flags) {
    int ret;
    epochEnd();
    ret = Real::mlockall(flags);
    epochBegin();
    return ret;
  }

  int munlockall() {
    int ret;
    epochEnd();
    ret = Real::munlockall();
    epochBegin();
    return ret;
  }

  int vhangup() {
    int ret;
    epochEnd();
    ret = Real::vhangup();
    epochBegin();
    return ret;
  }

  //  int modify_ldt(int func, void *ptr, unsigned long bytecount){
  // FIXME

  // pivot_root is not a libc function
  /*
  int pivot_root(const char *new_root, const char *put_old){
    // FIXME
    int ret;
    epochEnd();
    ret = Real::pivot_root(new_root, put_old);
    epochBegin();
    return ret;
  }*/

  int sysctl(int* name, int nlen, void* oldval, size_t* oldlenp, void* newval, size_t newlen) {
    int ret;
    epochEnd();
    ret = Real::sysctl(name, nlen, oldval, oldlenp, newval, newlen);
    epochBegin();
    return ret;
  }

  int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4,
            unsigned long arg5) {
    int ret;
    epochEnd();
    ret = Real::prctl(option, arg2, arg3, arg4, arg5);
    epochBegin();
    return ret;
  }

  // arch_prct isn't defined
  /*int arch_prctl(int code, unsigned long addr) {
    int ret;
    epochEnd();
    ret = Real::arch_prctl(code, addr);
    epochBegin();
    return ret;
  }*/

  int adjtimex(struct timex* buf) {
    int ret;
    epochEnd();
    epochBegin();
    ret = Real::adjtimex(buf);
    return ret;
  }

  int setrlimit(int resource, const struct rlimit* rlim) {
    int ret;
    epochEnd();
    ret = Real::setrlimit(resource, rlim);
    epochBegin();
    return ret;
  }

  // FIXME
  int chroot(const char* path) {
    int ret;
    epochEnd();
    ret = Real::chroot(path);
    epochBegin();
    return ret;
  }

  void sync() {
    // FIXME
    epochEnd();
    Real::sync();
    epochBegin();
  }

  int acct(const char* filename) {
    int ret;
    epochEnd();
    ret = Real::acct(filename);
    epochBegin();
    return ret;
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

    int ret;
    epochEnd();
    ret = Real::settimeofday(tv, tz);
    epochBegin();
    return ret;
  }

  int mount(const char* source, const char* target, const char* filesystemtype,
            unsigned long mountflags, const void* data) {
    // FIXME
    int ret;
    epochEnd();
    ret = Real::mount(source, target, filesystemtype, mountflags, data);
    epochBegin();
    return ret;
  }

  int umount2(const char* target, int flags) {
    // FIXME
    int ret;
    epochEnd();
    ret = Real::umount2(target, flags);
    epochBegin();
    return ret;
  }

  int swapon(const char* path, int swapflags) {

    int ret;
    epochEnd();
    ret = Real::swapon(path, swapflags);
    epochBegin();
    return ret;
  }

  int swapoff(const char* path) {

    int ret;
    epochEnd();
    ret = Real::swapoff(path);
    epochBegin();
    return ret;
  }

  int reboot(int cmd) {
    int ret;
    epochEnd();
    ret = Real::reboot(cmd);
    epochBegin();
    return ret;
  }

  int sethostname(const char* name, size_t len) {
    int ret;
    epochEnd();

    ret = Real::sethostname(name, len);
    epochBegin();
    return ret;
  }

  int setdomainname(const char* name, size_t len) {

    int ret;
    epochEnd();
    ret = Real::setdomainname(name, len);
    epochBegin();
    return ret;
  }

  int iopl(int level) {

    int ret;
    epochEnd();
    ret = Real::iopl(level);
    epochBegin();
    return ret;
  }

  int ioperm(unsigned long from, unsigned long num, int turn_on) {

    int ret;
    epochEnd();
    ret = Real::ioperm(from, num, turn_on);
    epochBegin();
    return ret;
  }

  // gettid is not a libc function
  /*pid_t gettid(){
    pid_t ret;
    epochEnd();
    ret = Real::gettid;
    epochBegin();
    return ret;
  }*/

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
    ssize_t ret;
    // epochEnd();
    ret = Real::readahead(fd, offset, count);
    // epochBegin();
    return ret;
  }

  int setxattr(const char* path, const char* name, const void* value, size_t size, int flags) {
    int ret;
    epochEnd();
    ret = Real::setxattr(path, name, value, size, flags);
    epochBegin();
    return ret;
  }

  int lsetxattr(const char* path, const char* name, const void* value, size_t size, int flags) {
    int ret;
    epochEnd();
    ret = Real::lsetxattr(path, name, value, size, flags);
    epochBegin();
    return ret;
  }

  int fsetxattr(int filedes, const char* name, const void* value, size_t size, int flags) {
    int ret;
    epochEnd();
    ret = Real::fsetxattr(filedes, name, value, size, flags);
    epochBegin();
    return ret;
  }

  ssize_t getxattr(const char* path, const char* name, void* value, size_t size) {
    ssize_t ret;
    epochEnd();
    ret = Real::getxattr(path, name, value, size);
    epochBegin();
    return ret;
  }

  ssize_t lgetxattr(const char* path, const char* name, void* value, size_t size) {
    ssize_t ret;
    epochEnd();
    ret = Real::lgetxattr(path, name, value, size);
    epochBegin();
    return ret;
  }

  ssize_t fgetxattr(int filedes, const char* name, void* value, size_t size) {
    ssize_t ret;
    epochEnd();
    ret = Real::fgetxattr(filedes, name, value, size);
    epochBegin();
    return ret;
  }

  ssize_t listxattr(const char* path, char* list, size_t size) {
    ssize_t ret;
    epochEnd();
    ret = Real::listxattr(path, list, size);
    epochBegin();
    return ret;
  }

  ssize_t llistxattr(const char* path, char* list, size_t size) {
    ssize_t ret;
    epochEnd();
    ret = Real::llistxattr(path, list, size);
    epochBegin();
    return ret;
  }

  ssize_t flistxattr(int filedes, char* list, size_t size) {
    ssize_t ret;
    epochEnd();
    ret = Real::flistxattr(filedes, list, size);
    epochBegin();
    return ret;
  }

  int removexattr(const char* path, const char* name) {
    int ret;
    epochEnd();
    ret = Real::removexattr(path, name);
    epochBegin();
    return ret;
  }

  int lremovexattr(const char* path, const char* name) {
    int ret;
    epochEnd();
    ret = Real::lremovexattr(path, name);
    epochBegin();
    return ret;
  }

  int fremovexattr(int filedes, const char* name) {
    int ret;
    epochEnd();
    ret = Real::fremovexattr(filedes, name);
    epochBegin();
    return ret;
  }

  int tkill(int /* tid */, int /* sig */) { return 0; }

  // Record this later.
  time_t time(time_t* t) {
    time_t ret;

    if(!doubletake::inRollback) {
      ret = Real::time(t);
      // Add this to the record list.
      _sysrecord.recordTimeOps(ret);
    } else {
      if(!_sysrecord.getTimeOps(&ret)) {
        assert(0);
      }
    }
    return ret;
  }

  // futex isn't defined
  /*int futex(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3){
    int ret;
    epochEnd();
    ret = Real::futex(uaddr, op, val, timeout, uaddr2, val3);
    epochBegin();
    return ret;
  }*/

  int sched_setaffinity(__pid_t pid, size_t cpusetsize, const cpu_set_t* mask) {
    int ret;
    epochEnd();
    ret = Real::sched_setaffinity(pid, cpusetsize, mask);
    epochBegin();
    return ret;
  }

  ssize_t sched_getaffinity(__pid_t pid, size_t cpusetsize, cpu_set_t* mask) {
    ssize_t ret;
    epochEnd();
    ret = Real::sched_getaffinity(pid, cpusetsize, mask);
    epochBegin();
    return ret;
  }

  // set_thread_area isn't a libc function
  /*int set_thread_area (struct user_desc *u_info){
    int ret;
    ret = Real::set_thread_area(u_info);
    epochBegin();
    return ret;
  }*/

  // io_* functions are not in libc
  /*int io_setup (int maxevents, io_context_t *ctxp){
    ret = Real::io_setup(maxevents, ctxp);
  }

  int io_destroy (io_context_t ctx){
    ret = Real::io_destroy(ctx);
  }

  long io_getevents (aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct
  timespec *timeout){
    ret = Real::io_getevents(ctx_id, min_nr, nr, events, timeout);
  }

  long io_submit (aio_context_t ctx_id, long nr, struct iocb **iocbpp){
    ret = Real::io_submit(ctx_id, nr, iocbpp);
  }

  long io_cancel (aio_context_t ctx_id, struct iocb *iocb, struct io_event *result){
    ret = Real::io_cancel(ctx_id, nr, iocbpp);
  }*/

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

  // get_thread_area is not a libc function
  /*int get_thread_area(struct user_desc *u_info){
    int ret;
    epochEnd();
    ret = Real::get_thread_area(u_info);
    epochBegin();
    return ret;
  }*/

  //  int lookup_dcookie(u64 cookie, char * buffer, size_t len){
  //    ret = WRAP(lookup_dcookie(cookie, buffer, len);
  //  }

  int epoll_create(int size) {

    int ret;
    epochEnd();
    ret = Real::epoll_create(size);
    epochBegin();
    return ret;
  }

  int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event) {

    int ret;
    epochEnd();
    ret = Real::epoll_ctl(epfd, op, fd, event);
    epochBegin();
    return ret;
  }

  int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout) {
    int ret = EINTR;
    while (ret == EINTR)
      ret = Real::epoll_wait(epfd, events, maxevents, timeout);
    epochEnd();
    epochBegin();
    return ret;
  }

  // Record it in the future.
  int remap_file_pages(void* start, size_t size, int prot, size_t pgoff, int flags) {
    int ret;
    epochEnd();
    ret = Real::remap_file_pages(start, size, prot, pgoff, flags);
    epochBegin();
    return ret;
  }

  // sys_set_tid_address is not defined
  /*long sys_set_tid_address (int *tidptr){
    long ret;
    epochEnd();
    ret = Real::sys_set_tid_address(tidptr);
    epochBegin();
    return ret;
  }*/

  // sys_set_tid_address is not defined
  /*long sys_restart_syscall(){
    long ret;
    epochEnd();
    ret = Real::sys_restart_syscall;
    epochBegin();
    return ret;
  }*/

  int semtimedop(int semid, struct sembuf* sops, size_t nsops, const struct timespec* timeout) {
    int ret;
    epochEnd();

    ret = Real::semtimedop(semid, sops, nsops, timeout);
    epochBegin();
    return ret;
  }

  long posix_fadvise64(int fs, loff_t offset, loff_t len, int advice) {
    long ret;
    epochEnd();
    ret = Real::posix_fadvise64(fs, offset, len, advice);
    epochBegin();
    return ret;
  }

  long timer_create(clockid_t which_clock, struct sigevent* timer_event_spec,
                    timer_t* created_timer_id) {
    long ret;
    epochEnd();

    ret = Real::timer_create(which_clock, timer_event_spec, created_timer_id);
    epochBegin();
    return ret;
  }

  long timer_settime(timer_t timer_id, int flags, const struct itimerspec* new_setting,
                     struct itimerspec* old_setting) {

    long ret;
    epochEnd();
    ret = Real::timer_settime(timer_id, flags, new_setting, old_setting);
    epochBegin();
    return ret;
  }

  long timer_gettime(timer_t timer_id, struct itimerspec* setting) {
    long ret;
    epochEnd();

    ret = Real::timer_gettime(timer_id, setting);
    epochBegin();
    return ret;
  }

  long timer_getoverrun(timer_t timer_id) {
    long ret;
    epochEnd();
    ret = Real::timer_getoverrun(timer_id);
    epochBegin();
    return ret;
  }

  long timer_delete(timer_t timer_id) {

    long ret;
    epochEnd();
    ret = Real::timer_delete(timer_id);
    epochBegin();
    return ret;
  }

  long clock_settime(clockid_t which_clock, const struct timespec* tp) {
    long ret;
    epochEnd();
    ret = Real::clock_settime(which_clock, tp);
    epochBegin();
    return ret;
  }

  long clock_gettime(clockid_t which_clock, struct timespec* tp) {
    long ret;
    epochEnd();
    ret = Real::clock_gettime(which_clock, tp);
    epochBegin();
    return ret;
  }

  long clock_getres(clockid_t which_clock, struct timespec* tp) {
    long ret;
    epochEnd();
    ret = Real::clock_getres(which_clock, tp);
    epochBegin();
    return ret;
  }

  long clock_nanosleep(clockid_t which_clock, int flags, const struct timespec* rqtp,
                       struct timespec* rmtp) {
    long ret;
    epochEnd();
    ret = Real::clock_nanosleep(which_clock, flags, rqtp, rmtp);
    epochBegin();
    return ret;
  }

  // exit_group is not defined
  /*void exit_group(int status){
    epochEnd();
    Real::exit_group(status);
    epochBegin();
  }*/

  // tgkill is not a libc function
  /*long sys_tgkill (int tgid, int pid, int sig){
    long ret;
    epochEnd();
    ret = Real::sys_tgkill(tgid, pid, sig);
    epochBegin();
    return ret;
  }*/

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
    int ret;
    epochEnd();
    ret = Real::utimes(filename, times);
    epochBegin();
    return ret;
  }

  mqd_t mq_open(const char* name, int oflag, mode_t mode, struct mq_attr* attr) {
    mqd_t ret;
    epochEnd();
    ret = Real::mq_open(name, oflag, mode, attr);
    epochBegin();
    return ret;
  }

  mqd_t mq_unlink(const char* name) {
    mqd_t ret;
    epochEnd();
    ret = Real::mq_unlink(name);
    epochBegin();
    return ret;
  }

  mqd_t mq_timedsend(mqd_t mqdes, const char* msg_ptr, size_t msg_len, unsigned msg_prio,
                     const struct timespec* abs_timeout) {
    mqd_t ret;
    epochEnd();
    ret = Real::mq_timedsend(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
    epochBegin();
    return ret;
  }

  mqd_t mq_timedreceive(mqd_t mqdes, char* msg_ptr, size_t msg_len, unsigned* msg_prio,
                        const struct timespec* abs_timeout) {
    mqd_t ret;
    epochEnd();
    ret = Real::mq_timedreceive(mqdes, msg_ptr, msg_len, msg_prio, abs_timeout);
    epochBegin();
    return ret;
  }

  mqd_t mq_notify(mqd_t mqdes, const struct sigevent* notification) {
    mqd_t ret;
    epochEnd();
    ret = Real::mq_notify(mqdes, notification);
    epochBegin();
    return ret;
  }

  // mq_getsetattr is not a libc function
  /*mqd_t mq_getsetattr(mqd_t mqdes, struct mq_attr *newattr,
                   struct mq_attr *oldattr){
    mqd_t ret;
    epochEnd();
    ret = Real::mq_getsetattr(mqdes, newattr, oldattr);
    epochBegin();
    return ret;
  }

  long kexec_load(void* entry, unsigned long nr_segments,
                 struct kexec_segment *segments, unsigned long flags){
    long ret;
    epochEnd();
    ret = Real::kexec_load(entry, nr_segments, segments, flags);
    epochBegin();
    return ret;
  }
*/

  int waitid(idtype_t idtype, id_t id, siginfo_t* infop, int options) {
    int ret;
    epochEnd();
    ret = Real::waitid(idtype, id, infop, options);
    epochBegin();
    return ret;
  }

  // key functions do not seem to exist
  /*key_serial_t add_key(const char *type, const char *description,
                       const void *payload, size_t plen, key_serial_t keyring) {
    int ret;
    epochEnd();
    ret = Real::add_key(type, description, payload, plen, keyring);
    epochBegin();
    return ret;
  }

  key_serial_t request_key(const char *type, const char *description,
                           const char *callout_info, key_serial_t keyring){
    ret = Real::request_key(type, description, callout_info, keyring);
    epochBegin();
    return ret;
  }

  long keyctl(int cmd, ...){
    // FIXME
    PRINF("keyctl is not supported now\n");
    return 0;
  }*/

  // ioprio_* functions are not in libc
  /*int ioprio_get(int which, int who){
    int ret;
    epochEnd();
    ret = Real::ioprio_get(which, who);
    epochBegin();
    return ret;
  }

  int ioprio_set(int which, int who, int ioprio){
    int ret;
    epochEnd();
    ret = Real::ioprio_set(which, who, ioprio);
    epochBegin();
    return ret;
  }*/

  int inotify_init() {
    int ret;
    epochEnd();
    ret = Real::inotify_init();
    epochBegin();
    return ret;
  }

  int inotify_add_watch(int fd, const char* pathname, uint32_t mask) {
    int ret;
    epochEnd();
    ret = Real::inotify_add_watch(fd, pathname, mask);
    epochBegin();
    return ret;
  }

  int inotify_rm_watch(int fd, uint32_t wd) {
    int ret;
    epochEnd();
    ret = Real::inotify_rm_watch(fd, wd);
    epochBegin();
    return ret;
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

  // int openat(int dirfd, const char *pathname, int flags){

  int openat(int dirfd, const char* pathname, int flags, mode_t mode) {
    int ret;
    epochEnd();
    ret = Real::openat(dirfd, pathname, flags, mode);
    epochBegin();
    return ret;
  }

  int mkdirat(int dirfd, const char* pathname, mode_t mode) {
    int ret;
    epochEnd();
    ret = Real::mkdirat(dirfd, pathname, mode);
    epochBegin();
    return ret;
  }

  int mknodat(int dirfd, const char* pathname, mode_t mode, dev_t dev) {
    int ret;
    epochEnd();
    ret = Real::mknodat(dirfd, pathname, mode, dev);
    epochBegin();
    return ret;
  }

  int fchownat(int dirfd, const char* path, uid_t owner, gid_t group, int flags) {
    int ret;
    epochEnd();
    ret = Real::fchownat(dirfd, path, owner, group, flags);
    epochBegin();
    return ret;
  }

  int futimesat(int dirfd, const char* path, const struct timeval times[2]) {
    int ret;
    epochEnd();
    ret = Real::futimesat(dirfd, path, times);
    epochBegin();
    return ret;
  }

  int unlinkat(int dirfd, const char* pathname, int flags) {
    int ret;
    epochEnd();
    ret = Real::unlinkat(dirfd, pathname, flags);
    epochBegin();
    return ret;
  }

  int renameat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath) {
    int ret;
    epochEnd();
    ret = Real::renameat(olddirfd, oldpath, newdirfd, newpath);
    epochBegin();
    return ret;
  }

  int linkat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags) {
    int ret;
    epochEnd();
    ret = Real::linkat(olddirfd, oldpath, newdirfd, newpath, flags);
    epochBegin();
    return ret;
  }

  int symlinkat(const char* oldpath, int newdirfd, const char* newpath) {
    int ret;
    epochEnd();
    ret = Real::symlinkat(oldpath, newdirfd, newpath);
    epochBegin();
    return ret;
  }

  int readlinkat(int dirfd, const char* path, char* buf, size_t bufsiz) {
    int ret;
    epochEnd();
    ret = Real::readlinkat(dirfd, path, buf, bufsiz);
    epochBegin();
    return ret;
  }

  int fchmodat(int dirfd, const char* path, mode_t mode, int flags) {
    int ret;
    epochEnd();
    ret = Real::fchmodat(dirfd, path, mode, flags);
    epochBegin();
    return ret;
  }

  int faccessat(int dirfd, const char* path, int mode, int flags) {
    int ret;
    epochEnd();
    ret = Real::faccessat(dirfd, path, mode, flags);
    epochBegin();
    return ret;
  }

  int pselect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
              const struct timespec* timeout, const sigset_t* sigmask) {
    int ret;
    epochEnd();
    ret = Real::pselect(nfds, readfds, writefds, exceptfds, timeout, sigmask);
    epochBegin();
    return ret;
  }

  int ppoll(struct pollfd* fds, nfds_t nfds, const struct timespec* timeout,
            const sigset_t* sigmask) {
    int ret;
    epochEnd();
    ret = Real::ppoll(fds, nfds, timeout, sigmask);
    epochBegin();
    return ret;
  }

  int unshare(int flags) {
    int ret;
    epochEnd();
    ret = Real::unshare(flags);
    epochBegin();
    return ret;
  }

  // get/set_robust_list functions are not in libc
  /*long get_robust_list(int pid, struct robust_list_head **head_ptr, size_t *len_ptr){
    long ret;
    epochEnd();
    ret = Real::get_robust_list(pid, head_ptr, len_ptr);
    epochBegin();
    return ret;
  }

  long set_robust_list(struct robust_list_head *head, size_t len){
    long ret;
    epochEnd();
    ret = Real::set_robust_list(head, len);
    epochBegin();
    return ret;
  }*/

  int splice(int fd_in, __off64_t* off_in, int fd_out, __off64_t* off_out, size_t len,
             unsigned int flags) {
    int ret;
    epochEnd();
    ret = Real::splice(fd_in, off_in, fd_out, off_out, len, flags);
    epochBegin();
    return ret;
  }

  int tee(int fd_in, int fd_out, size_t len, unsigned int flags) {
    int ret;
    epochEnd();
    ret = Real::tee(fd_in, fd_out, len, flags);
    epochBegin();
    return ret;
  }

  int sync_file_range(int fd, __off64_t offset, __off64_t nbytes, unsigned int flags) {
    int ret;
    epochEnd();
    ret = Real::sync_file_range(fd, offset, nbytes, flags);
    epochBegin();
    return ret;
  }

  int vmsplice(int fd, const struct iovec* iov, size_t nr_segs, unsigned int flags) {
    int ret;
    epochEnd();
    ret = Real::vmsplice(fd, iov, nr_segs, flags);
    epochBegin();
    return ret;
  }

  // move_pages isn't defined in any headers
  /*long move_pages(pid_t pid, unsigned long nr_pages,
                  const void **address,
                  const int *nodes, int *status,
                  int flags){
    long ret;
    epochEnd();
    ret = Real::move_pages(pid, nr_pages, address, nodes, status, flags);
    epochBegin();
    return ret;
  }*/

  // __clone isn't defined in any headers
  /*int __clone(int (*fn)(void *), void *child_stack, int flags, void *arg, pid_t *pid, struct
  user_desc * tls, pid_t *ctid) {
    int ret;

    if(!doubletake::inRollback) {
      ret = Real::__clone(fn, child_stack, flags, arg, pid, tls, ctid);
      _sysrecord.recordCloneOps(ret);
    }
    else {
      ret = _sysrecord.getCloneOps();
    }

    return ret;
  }*/

private:
	SysRecord _sysrecord;
  fops _fops;
};

#endif
