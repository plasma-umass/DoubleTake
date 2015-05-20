#if !defined(DOUBLETAKE_REAL_H)
#define DOUBLETAKE_REAL_H

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <malloc.h>
#include <mqueue.h>
#include <pthread.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/fsuid.h>
#include <sys/inotify.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/personality.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/sem.h>
#include <sys/sendfile.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/timex.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>
#include <ustat.h>
#include <utime.h>

#define DECLARE_WRAPPER(name) extern decltype(::name) * name;

namespace Real {
void initializer();

// No libc wrappers
// DECLARE_WRAPPER(getdents);
// DECLARE_WRAPPER(modify_ldt);
// DECLARE_WRAPPER(uselib);
// DECLARE_WRAPPER(sysfs);
// DECLARE_WRAPPER(gettid);
// DECLARE_WRAPPER(tkill);
// DECLARE_WRAPPER(set_thread_area);
// DECLARE_WRAPPER(get_thread_area);
// DECLARE_WRAPPER(io_setup);
// DECLARE_WRAPPER(io_destroy);
// DECLARE_WRAPPER(io_getevents);
// DECLARE_WRAPPER(io_submit);
// DECLARE_WRAPPER(io_cancel);
// DECLARE_WRAPPER(lookup_dcookie);
// DECLARE_WRAPPER(sys_tgkill);
// DECLARE_WRAPPER(mq_getsetattr);
// DECLARE_WRAPPER(ioprio_get);
// DECLARE_WRAPPER(ioprio_set);
// DECLARE_WRAPPER(get_robust_list);
// DECLARE_WRAPPER(set_robust_list);
// DECLARE_WRAPPER(pivot_root);
// DECLARE_WRAPPER(arch_prctl);
// DECLARE_WRAPPER(futex);
// DECLARE_WRAPPER(set_tid_address);
// DECLARE_WRAPPER(restart_syscall);
// DECLARE_WRAPPER(exit_group);
// DECLARE_WRAPPER(add_key);
// DECLARE_WRAPPER(request_key);
// DECLARE_WRAPPER(keyctl);
// DECLARE_WRAPPER(move_pages);
// DECLARE_WRAPPER(kexec_load);

DECLARE_WRAPPER(accept);
DECLARE_WRAPPER(access);
DECLARE_WRAPPER(acct);
DECLARE_WRAPPER(adjtimex);
DECLARE_WRAPPER(alarm);
DECLARE_WRAPPER(bind);
DECLARE_WRAPPER(brk);
DECLARE_WRAPPER(chdir);
DECLARE_WRAPPER(chmod);
DECLARE_WRAPPER(chown);
DECLARE_WRAPPER(chroot);
DECLARE_WRAPPER(clock_getres);
DECLARE_WRAPPER(clock_gettime);
DECLARE_WRAPPER(clock_nanosleep);
DECLARE_WRAPPER(clock_settime);
DECLARE_WRAPPER(clone);
DECLARE_WRAPPER(close);
DECLARE_WRAPPER(closedir);
DECLARE_WRAPPER(connect);
DECLARE_WRAPPER(creat);
DECLARE_WRAPPER(dup);
DECLARE_WRAPPER(dup2);
DECLARE_WRAPPER(epoll_create);
DECLARE_WRAPPER(epoll_ctl);
DECLARE_WRAPPER(epoll_wait);
DECLARE_WRAPPER(execve);
DECLARE_WRAPPER(exit);
DECLARE_WRAPPER(faccessat);
DECLARE_WRAPPER(fchdir);
DECLARE_WRAPPER(fchmod);
DECLARE_WRAPPER(fchmodat);
DECLARE_WRAPPER(fchown);
DECLARE_WRAPPER(fchownat);
DECLARE_WRAPPER(fclose);
DECLARE_WRAPPER(fcntl);
DECLARE_WRAPPER(fdatasync);
DECLARE_WRAPPER(fgetxattr);
DECLARE_WRAPPER(flistxattr);
DECLARE_WRAPPER(flock);
DECLARE_WRAPPER(printf);
DECLARE_WRAPPER(puts);
DECLARE_WRAPPER(fopen);
DECLARE_WRAPPER(fopen64);
DECLARE_WRAPPER(fork);
DECLARE_WRAPPER(fread);
DECLARE_WRAPPER(free);
DECLARE_WRAPPER(fremovexattr);
DECLARE_WRAPPER(fsetxattr);
DECLARE_WRAPPER(fstat);
DECLARE_WRAPPER(fstatfs);
DECLARE_WRAPPER(fsync);
DECLARE_WRAPPER(ftruncate);
DECLARE_WRAPPER(futimesat);
DECLARE_WRAPPER(fwrite);
DECLARE_WRAPPER(getcwd);
DECLARE_WRAPPER(getegid);
DECLARE_WRAPPER(geteuid);
DECLARE_WRAPPER(getgid);
DECLARE_WRAPPER(getgroups);
DECLARE_WRAPPER(getitimer);
DECLARE_WRAPPER(getpeername);
DECLARE_WRAPPER(getpgid);
DECLARE_WRAPPER(getpgrp);
DECLARE_WRAPPER(getpid);
DECLARE_WRAPPER(getppid);
DECLARE_WRAPPER(getpriority);
DECLARE_WRAPPER(getresgid);
DECLARE_WRAPPER(getresuid);
DECLARE_WRAPPER(getrlimit);
DECLARE_WRAPPER(getrusage);
DECLARE_WRAPPER(getsid);
DECLARE_WRAPPER(getsockname);
DECLARE_WRAPPER(getsockopt);
DECLARE_WRAPPER(gettimeofday);
DECLARE_WRAPPER(getuid);
DECLARE_WRAPPER(getxattr);
DECLARE_WRAPPER(inotify_add_watch);
DECLARE_WRAPPER(inotify_init);
DECLARE_WRAPPER(inotify_rm_watch);
DECLARE_WRAPPER(ioctl);
DECLARE_WRAPPER(ioperm);
DECLARE_WRAPPER(iopl);
DECLARE_WRAPPER(kill);
DECLARE_WRAPPER(lchown);
DECLARE_WRAPPER(lgetxattr);
DECLARE_WRAPPER(link);
DECLARE_WRAPPER(linkat);
DECLARE_WRAPPER(listen);
DECLARE_WRAPPER(listxattr);
DECLARE_WRAPPER(llistxattr);
DECLARE_WRAPPER(lremovexattr);
DECLARE_WRAPPER(lseek);
DECLARE_WRAPPER(lsetxattr);
DECLARE_WRAPPER(lstat);
DECLARE_WRAPPER(madvise);
DECLARE_WRAPPER(malloc);
DECLARE_WRAPPER(malloc_usable_size);
DECLARE_WRAPPER(memalign);
DECLARE_WRAPPER(mincore);
DECLARE_WRAPPER(mkdir);
DECLARE_WRAPPER(mkdirat);
DECLARE_WRAPPER(mknod);
DECLARE_WRAPPER(mknodat);
DECLARE_WRAPPER(mlock);
DECLARE_WRAPPER(mlockall);
DECLARE_WRAPPER(mmap);
DECLARE_WRAPPER(mount);
DECLARE_WRAPPER(mprotect);
DECLARE_WRAPPER(mq_notify);
DECLARE_WRAPPER(mq_open);
DECLARE_WRAPPER(mq_timedreceive);
DECLARE_WRAPPER(mq_timedsend);
DECLARE_WRAPPER(mq_unlink);
DECLARE_WRAPPER(mremap);
DECLARE_WRAPPER(msync);
DECLARE_WRAPPER(munlock);
DECLARE_WRAPPER(munlockall);
DECLARE_WRAPPER(munmap);
DECLARE_WRAPPER(nanosleep);
DECLARE_WRAPPER(open);
DECLARE_WRAPPER(openat);
DECLARE_WRAPPER(opendir);
DECLARE_WRAPPER(pause);
DECLARE_WRAPPER(personality);
DECLARE_WRAPPER(pipe);
DECLARE_WRAPPER(poll);
DECLARE_WRAPPER(posix_fadvise64);
DECLARE_WRAPPER(ppoll);
DECLARE_WRAPPER(prctl);
DECLARE_WRAPPER(pread);
DECLARE_WRAPPER(pselect);

DECLARE_WRAPPER(pthread_barrier_destroy);
DECLARE_WRAPPER(pthread_barrier_init);
DECLARE_WRAPPER(pthread_barrier_wait);
DECLARE_WRAPPER(pthread_cancel);
DECLARE_WRAPPER(pthread_condattr_init);
DECLARE_WRAPPER(pthread_cond_broadcast);
DECLARE_WRAPPER(pthread_cond_destroy);
DECLARE_WRAPPER(pthread_cond_init);
DECLARE_WRAPPER(pthread_cond_signal);
DECLARE_WRAPPER(pthread_cond_wait);
DECLARE_WRAPPER(pthread_cond_timedwait);
DECLARE_WRAPPER(pthread_create);
DECLARE_WRAPPER(pthread_detach);
DECLARE_WRAPPER(pthread_exit);
DECLARE_WRAPPER(pthread_join);
DECLARE_WRAPPER(pthread_kill);
DECLARE_WRAPPER(pthread_mutexattr_init);
DECLARE_WRAPPER(pthread_mutex_destroy);
DECLARE_WRAPPER(pthread_mutex_init);
DECLARE_WRAPPER(pthread_mutex_lock);
DECLARE_WRAPPER(pthread_mutex_trylock);
DECLARE_WRAPPER(pthread_mutex_unlock);
DECLARE_WRAPPER(pthread_self);

DECLARE_WRAPPER(ptrace);
DECLARE_WRAPPER(pwrite);
DECLARE_WRAPPER(read);
DECLARE_WRAPPER(readahead);
DECLARE_WRAPPER(readlink);
DECLARE_WRAPPER(readlinkat);
DECLARE_WRAPPER(readv);
DECLARE_WRAPPER(realloc);
DECLARE_WRAPPER(reboot);
DECLARE_WRAPPER(recvfrom);
DECLARE_WRAPPER(recvmsg);
DECLARE_WRAPPER(remap_file_pages);
DECLARE_WRAPPER(removexattr);
DECLARE_WRAPPER(rename);
DECLARE_WRAPPER(renameat);
DECLARE_WRAPPER(rmdir);
DECLARE_WRAPPER(sched_getaffinity);
DECLARE_WRAPPER(sched_getparam);
DECLARE_WRAPPER(sched_getscheduler);
DECLARE_WRAPPER(sched_get_priority_max);
DECLARE_WRAPPER(sched_get_priority_min);
DECLARE_WRAPPER(sched_rr_get_interval);
DECLARE_WRAPPER(sched_setaffinity);
DECLARE_WRAPPER(sched_setparam);
DECLARE_WRAPPER(sched_setscheduler);
DECLARE_WRAPPER(sched_yield);
DECLARE_WRAPPER(select);
DECLARE_WRAPPER(semctl);
DECLARE_WRAPPER(semget);
DECLARE_WRAPPER(semop);
DECLARE_WRAPPER(semtimedop);
DECLARE_WRAPPER(sendfile);
DECLARE_WRAPPER(sendmsg);
DECLARE_WRAPPER(sendto);
DECLARE_WRAPPER(setdomainname);
DECLARE_WRAPPER(setfsgid);
DECLARE_WRAPPER(setfsuid);
DECLARE_WRAPPER(setgid);
DECLARE_WRAPPER(setgroups);
DECLARE_WRAPPER(sethostname);
DECLARE_WRAPPER(setitimer);
DECLARE_WRAPPER(setpgid);
DECLARE_WRAPPER(setpriority);
DECLARE_WRAPPER(setregid);
DECLARE_WRAPPER(setresgid);
DECLARE_WRAPPER(setresuid);
DECLARE_WRAPPER(setreuid);
DECLARE_WRAPPER(setrlimit);
DECLARE_WRAPPER(setsid);
DECLARE_WRAPPER(setsockopt);
DECLARE_WRAPPER(settimeofday);
DECLARE_WRAPPER(setuid);
DECLARE_WRAPPER(setxattr);
DECLARE_WRAPPER(shmat);
DECLARE_WRAPPER(shmctl);
DECLARE_WRAPPER(shmget);
DECLARE_WRAPPER(shutdown);
DECLARE_WRAPPER(sigaction);
DECLARE_WRAPPER(sigaltstack);
DECLARE_WRAPPER(sigpending);
DECLARE_WRAPPER(sigprocmask);
DECLARE_WRAPPER(sigreturn);
DECLARE_WRAPPER(sigsuspend);
DECLARE_WRAPPER(sigtimedwait);
DECLARE_WRAPPER(sigwait);
DECLARE_WRAPPER(socket);
DECLARE_WRAPPER(socketpair);
DECLARE_WRAPPER(splice);
DECLARE_WRAPPER(stat);
DECLARE_WRAPPER(statfs);
DECLARE_WRAPPER(swapoff);
DECLARE_WRAPPER(swapon);
DECLARE_WRAPPER(symlink);
DECLARE_WRAPPER(symlinkat);
DECLARE_WRAPPER(sync);
DECLARE_WRAPPER(sync_file_range);
DECLARE_WRAPPER(sysctl);
DECLARE_WRAPPER(sysinfo);
DECLARE_WRAPPER(vsyslog);
DECLARE_WRAPPER(tee);
DECLARE_WRAPPER(time);
DECLARE_WRAPPER(timer_create);
DECLARE_WRAPPER(timer_delete);
DECLARE_WRAPPER(timer_getoverrun);
DECLARE_WRAPPER(timer_gettime);
DECLARE_WRAPPER(timer_settime);
DECLARE_WRAPPER(times);
DECLARE_WRAPPER(truncate);
DECLARE_WRAPPER(umask);
DECLARE_WRAPPER(umount2);
DECLARE_WRAPPER(uname);
DECLARE_WRAPPER(unlink);
DECLARE_WRAPPER(unlinkat);
DECLARE_WRAPPER(unshare);
DECLARE_WRAPPER(ustat);
DECLARE_WRAPPER(utime);
DECLARE_WRAPPER(utimes);
DECLARE_WRAPPER(vfork);
DECLARE_WRAPPER(vhangup);
DECLARE_WRAPPER(vmsplice);
DECLARE_WRAPPER(wait4);
DECLARE_WRAPPER(waitid);
DECLARE_WRAPPER(write);
DECLARE_WRAPPER(writev);
};

#endif
