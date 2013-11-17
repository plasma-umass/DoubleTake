#ifndef _REAL_H_
#define _REAL_H_

#if 1
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
//#ifdef HANDLE_SYSCALL
#include <fcntl.h>
#include <grp.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
//#include <attr/xattr.h>
//#include <mqueue.h>
//#include <keyutils.h>
//#include <aio.h>
//#include <linux/futex.h>
//#include <linux/unistd.h>
#include <linux/sysctl.h>
#include <linux/reboot.h>
#include <sys/epoll.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/msg.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>

#endif

#include <sys/types.h>

#define WRAP(x) _real_##x

// libc functions
extern void* (*WRAP(mmap))(void*, size_t, int, int, int, off_t);
extern void* (*WRAP(malloc))(size_t);
extern void  (*WRAP(free))(void *);
extern void* (*WRAP(realloc))(void *, size_t);
extern void* (*WRAP(memalign))(size_t, size_t);
extern size_t (*WRAP(malloc_usable_size))(void *);
extern ssize_t (*WRAP(read))(int, void*, size_t);
extern ssize_t (*WRAP(write))(int, const void*, size_t);
extern int (*WRAP(sigwait))(const sigset_t*, int*);
extern int (*WRAP(__clone))(int (*fn)(void *), void *child_stack, int flags, void *arg, pid_t *pid, struct user_desc * tls, pid_t *ctid);

// pthread basics
extern int (*WRAP(pthread_create))(pthread_t*, const pthread_attr_t*, void *(*)(void*), void*);
extern int (*WRAP(pthread_cancel))(pthread_t);
extern int (*WRAP(pthread_detach))(pthread_t);
extern int (*WRAP(pthread_kill))(pthread_t, int);
extern int (*WRAP(pthread_join))(pthread_t, void**);
extern pthread_t (*WRAP(pthread_self))();
extern int (*WRAP(pthread_exit))(void*);

// pthread mutexes
extern int (*WRAP(pthread_mutexattr_init))(pthread_mutexattr_t*);
extern int (*WRAP(pthread_mutex_init))(pthread_mutex_t*, const pthread_mutexattr_t*);
extern int (*WRAP(pthread_mutex_lock))(pthread_mutex_t*);
extern int (*WRAP(pthread_mutex_unlock))(pthread_mutex_t*);
extern int (*WRAP(pthread_mutex_trylock))(pthread_mutex_t*);
extern int (*WRAP(pthread_mutex_destroy))(pthread_mutex_t*);

// pthread condition variables
extern int (*WRAP(pthread_condattr_init))(pthread_condattr_t*);
extern int (*WRAP(pthread_cond_init))(pthread_cond_t*, const pthread_condattr_t*);
extern int (*WRAP(pthread_cond_wait))(pthread_cond_t*, pthread_mutex_t*);
extern int (*WRAP(pthread_cond_signal))(pthread_cond_t*);
extern int (*WRAP(pthread_cond_broadcast))(pthread_cond_t*);
extern int (*WRAP(pthread_cond_destroy))(pthread_cond_t*);

// pthread barriers
extern int (*WRAP(pthread_barrier_init))(pthread_barrier_t*, const pthread_barrierattr_t*, unsigned int);
extern int (*WRAP(pthread_barrier_wait))(pthread_barrier_t*);
extern int (*WRAP(pthread_barrier_destroy))(pthread_barrier_t*);

//System call realted functions
extern time_t  (*WRAP(time))(time_t *);
extern int (*WRAP(gettimeofday))(struct timeval *, struct timezone *);

void init_real_functions();

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

//#ifdef HANDLE_SYSCALL
//#if 1

extern ssize_t (*WRAP(read))(int, void*, size_t);
extern ssize_t (*WRAP(write))(int, const void*, size_t);
extern void* (*WRAP(mmap))(void*, size_t, int, int, int, off_t);
extern int (*WRAP(open))(const char *pathname, int flags, mode_t mode);
extern int (*WRAP(close))(int fd);
extern DIR * (*WRAP(opendir))(const char *name);
extern int (*WRAP(closedir))(DIR *dir);
extern FILE* (*WRAP(fopen))(const char*pathname, const char* modes);
extern FILE* (*WRAP(fopen64))(const char*pathname, const char* modes);
extern int (*WRAP(fclose))(FILE *fp);
extern size_t (*WRAP(fread))(void *ptr, size_t size, size_t nmemb, FILE *stream);
extern size_t (*WRAP(fwrite))(const void *ptr, size_t size, size_t nmemb, FILE *stream);
extern int (*WRAP(stat))(const char *path, struct stat *buf);
extern int (*WRAP(fstat))(int filedes, struct stat *buf);
extern int (*WRAP(lstat))(const char *path, struct stat *buf);
extern int (*WRAP(poll))(struct pollfd *fds, nfds_t nfds, int timeout);
extern off_t (*WRAP(lseek))(int fildes, off_t offset, int whence);
extern int (*WRAP(mprotect))(const void *addr, size_t len, int prot);
extern int (*WRAP(munmap))(void *start, size_t length);
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
extern int (*WRAP(brk))(void *end_data_segment);
extern int (*WRAP(sigaction))(int signum, const struct sigaction *act, struct sigaction *oldact);
extern int (*WRAP(sigprocmask))(int how, const sigset_t *set, sigset_t *oldset);
extern int (*WRAP(sigreturn))(unsigned long __unused);
extern int (*WRAP(ioctl))(int d, int request, ...);
extern ssize_t (*WRAP(pread))(int fd, void *buf, size_t count, off_t offset);
extern ssize_t (*WRAP(pwrite))(int fd, const void *buf, size_t count, off_t offset);
extern ssize_t (*WRAP(readv))(int fd, const struct iovec *vector, int count);
extern ssize_t (*WRAP(writev))(int fd, const struct iovec *vector, int count);
extern int (*WRAP(access))(const char *pathname, int mode);
extern int (*WRAP(pipe))(int filedes[2]);
extern int (*WRAP(select))(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
//extern int sched_yield(void);
extern void * (*WRAP(mremap))(void *old_address, size_t old_size , size_t new_size, int flags);
extern int (*WRAP(msync))(void *start, size_t length, int flags);
extern int (*WRAP(mincore))(void *start, size_t length, unsigned char *vec);
extern int (*WRAP(madvise))(void *start, size_t length, int advice);
extern int (*WRAP(shmget))(key_t key, size_t size, int shmflg);
extern void *(*WRAP(shmat))(int shmid, const void *shmaddr, int shmflg);
extern int (*WRAP(shmctl))(int shmid, int cmd, struct shmid_ds *buf);

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
extern int (*WRAP(dup))(int oldfd);
extern int (*WRAP(dup2))(int oldfd, int newfd);
extern int (*WRAP(pause))(void); // FIXME
extern int (*WRAP(nanosleep))(const struct timespec *req, struct timespec *rem); //FIXME
extern int (*WRAP(getitimer))(int which, struct itimerval *value);
extern unsigned int (*WRAP(alarm))(unsigned int seconds); //FIXME
extern int (*WRAP(setitimer))(int which, const struct itimerval *value, struct itimerval *ovalue);
//extern pid_t getpid(void); //FIXME
extern ssize_t (*WRAP(sendfile))(int out_fd, int in_fd, off_t *offset, size_t count);
extern int (*WRAP(socket))(int domain, int type, int protocol);
extern int (*WRAP(connect))(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
extern int (*WRAP(accept))(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern ssize_t  (*WRAP(sendto))(int  s,  const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
extern ssize_t (*WRAP(recvfrom))(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
extern ssize_t (*WRAP(sendmsg))(int s, const struct msghdr *msg, int flags);
extern ssize_t (*WRAP(recvmsg))(int s, struct msghdr *msg, int flags);
extern int (*WRAP(shutdown))(int s, int how); //FIXME
extern int (*WRAP(bind))(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);
extern int (*WRAP(listen))(int sockfd, int backlog);
extern int (*WRAP(getsockname))(int s, struct sockaddr *name, socklen_t *namelen);
extern int (*WRAP(getpeername))(int s, struct sockaddr *name, socklen_t *namelen);
extern int (*WRAP(socketpair))(int d, int type, int protocol, int sv[2]);
extern int (*WRAP(setsockopt))(int s, int level, int optname, const void *optval, socklen_t optlen);
extern int (*WRAP(getsockopt))(int s, int level, int optname, void *optval, socklen_t *optlen);
extern int (*WRAP(clone))(int (*fn)(void *), void *child_stack, int flags, void *arg, ... /* pid_t *pid, struct user_desc *tls, pid_t *ctid */ );
extern pid_t (*WRAP(fork))(void);
extern pid_t (*WRAP(vfork))(void);
extern int (*WRAP(execve))(const char *filename, char *const argv[], char *const envp[]);
extern void (*WRAP(exit))(int status);
extern pid_t (*WRAP(wait4))(pid_t pid, void *status, int options, struct rusage *rusage);
extern int (*WRAP(kill))(pid_t pid, int sig);
extern int (*WRAP(uname))(struct utsname *buf);

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
extern int (*WRAP(semget))(key_t key, int nsems, int semflg);
extern int (*WRAP(semop))(int semid, struct sembuf *sops, size_t nsops);
extern int (*WRAP(semctl))(int semid, int semnum, int cmd, ...);
extern int (*WRAP(fcntl))(int fd, int cmd, long arg);
extern int (*WRAP(flock))(int fd, int operation);
extern int (*WRAP(fsync))(int fd);
extern int (*WRAP(fdatasync))(int fd);
extern int (*WRAP(truncate))(const char *path, off_t length);
extern int (*WRAP(ftruncate))(int fd, off_t length);
extern int (*WRAP(getdents))(unsigned int fd, struct dirent *dirp, unsigned int count);
extern char * (*WRAP(getcwd))(char *buf, unsigned long size);
extern int (*WRAP(chdir))(const char *path);
extern int (*WRAP(fchdir))(int fd);

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
extern int (*WRAP(rename))(const char *oldpath, const char *newpath);
extern int (*WRAP(mkdir))(const char *pathname, mode_t mode);
extern int (*WRAP(rmdir))(const char *pathname);
extern int (*WRAP(creat))(const char *pathname, mode_t mode);
extern int (*WRAP(link))(const char *oldpath, const char *newpath);
extern int (*WRAP(unlink))(const char *pathname);
extern int (*WRAP(symlink))(const char *oldpath, const char *newpath);
extern ssize_t (*WRAP(readlink))(const char *path, char *buf, size_t bufsiz);
extern int (*WRAP(chmod))(const char *path, mode_t mode);
extern int (*WRAP(fchmod))(int fildes, mode_t mode);
extern int (*WRAP(chown))(const char *path, uid_t owner, gid_t group);
extern int (*WRAP(fchown))(int fd, uid_t owner, gid_t group);
extern int (*WRAP(lchown))(const char *path, uid_t owner, gid_t group);
extern mode_t (*WRAP(umask))(mode_t mask);
extern int (*WRAP(gettimeofday))(struct timeval *tv, struct timezone *tz);
//extern int (*WRAP(settimeofday))(const struct timeval *tv , const struct timezone *tz);
extern int (*WRAP(getrlimit))(int resource, struct rlimit *rlim);
extern int (*WRAP(getrusage))(int who, struct rusage *usage);
extern int (*WRAP(sysinfo))(struct sysinfo *info);
extern clock_t (*WRAP(times))(struct tms *buf);
//extern long ptrace(enum __ptrace_request request, pid_t pid, void *addr, void *data); //FIXME
extern uid_t (*WRAP(getuid))(void);
extern int (*WRAP(syslog))(int type, char *bufp, int len);

/*
AP(fine _SYS_setuid                            105
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

extern gid_t (*WRAP(getgid))(void);
extern int (*WRAP(setuid))(uid_t uid);
extern int (*WRAP(setgid))(gid_t gid);
extern uid_t (*WRAP(geteuid))(void);
extern gid_t (*WRAP(getegid))(void);
extern int (*WRAP(setpgid))(pid_t pid, pid_t pgid);
extern pid_t (*WRAP(getppid))(void);
extern pid_t (*WRAP(getpgrp))(void);
extern pid_t (*WRAP(setsid))(void);
extern int (*WRAP(setreuid))(uid_t ruid, uid_t euid);
extern int (*WRAP(setregid))(gid_t rgid, gid_t egid);
extern int (*WRAP(getgroups))(int size, gid_t list[]);
extern int (*WRAP(setgroups))(size_t size, const gid_t *list);
extern int (*WRAP(setresuid))(uid_t ruid, uid_t euid, uid_t suid);
extern int (*WRAP(getresuid))(uid_t *ruid, uid_t *euid, uid_t *suid);
extern int (*WRAP(setresgid))(gid_t rgid, gid_t egid, gid_t sgid);
extern int (*WRAP(getresgid))(gid_t *rgid, gid_t *egid, gid_t *sgid);
extern pid_t (*WRAP(getpgid))(pid_t pid);
extern int (*WRAP(setfsuid))(uid_t fsuid);
extern int (*WRAP(setfsgid))(uid_t fsgid);

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

extern pid_t (*WRAP(getsid))(pid_t pid);
extern int (*WRAP(sigpending))(sigset_t *set);
extern int (*WRAP(sigtimedwait))(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
extern int (*WRAP(sigsuspend))(const sigset_t *mask);
extern int (*WRAP(sigaltstack))(const stack_t *ss, stack_t *oss);
extern int (*WRAP(utime))(const char *filename, const struct utimbuf *buf);
extern int (*WRAP(mknod))(const char *pathname, mode_t mode, dev_t dev);
extern int (*WRAP(uselib))(const char *library);
extern int (*WRAP(personality))(unsigned long persona);
extern int (*WRAP(ustat))(dev_t dev, struct ustat *ubuf);
extern int (*WRAP(statfs))(const char *path, struct statfs *buf);
extern int (*WRAP(fstatfs))(int fd, struct statfs *buf);
extern int (*WRAP(sysfs))(int option, unsigned int fs_index, char *buf);
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
extern int (*WRAP(getpriority))(int which, int who);
extern int (*WRAP(setpriority))(int which, int who, int prio);
extern int (*WRAP(sched_setparam))(pid_t pid, const struct sched_param *param);
extern int (*WRAP(sched_getparam))(pid_t pid, struct sched_param *param);
extern int (*WRAP(sched_setscheduler))(pid_t pid, int policy, const struct sched_param *param);
extern int (*WRAP(sched_getscheduler))(pid_t pid);
extern int (*WRAP(sched_get_priority_max))(int policy);
extern int (*WRAP(sched_get_priority_min))(int policy);
extern int (*WRAP(sched_rr_get_interval))(pid_t pid, struct timespec *tp);
extern int (*WRAP(mlock))(const void *addr, size_t len);
extern int (*WRAP(munlock))(const void *addr, size_t len);
extern int (*WRAP(mlockall))(int flags);
extern int (*WRAP(munlockall))(void);
extern int (*WRAP(vhangup))(void);
extern int (*WRAP(modify_ldt))(int func, void *ptr, unsigned long bytecount);
extern int (*WRAP(pivot_root))(const char *new_root, const char *put_old);
extern int (*WRAP(_sysctl))(struct __sysctl_args *args);
extern int (*WRAP( prctl))(int  option,  unsigned  long arg2, unsigned long arg3 , unsigned long arg4, unsigned long arg5);
extern int (*WRAP(arch_prctl))(int code, unsigned long addr);
extern int (*WRAP(adjtimex))(struct timex *buf);
extern int (*WRAP(setrlimit))(int resource, const struct rlimit *rlim);
extern int (*WRAP(chroot))(const char *path);
extern void(*WRAP( sync))(void);
extern int (*WRAP(acct))(const char *filename);


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
extern int (*WRAP(settimeofday))(const struct timeval *tv , const struct timezone *tz);
extern int (*WRAP(mount))(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data);
extern int (*WRAP(umount2))(const char *target, int flags);
extern int (*WRAP(swapon))(const char *path, int swapflags);
extern int (*WRAP(swapoff))(const char *path);
extern int (*WRAP(reboot))(int magic, int magic2, int flag, void *arg);
extern int (*WRAP(sethostname))(const char *name, size_t len);
extern int (*WRAP(setdomainname))(const char *name, size_t len);
extern int (*WRAP(iopl))(int level);
extern int (*WRAP(ioperm))(unsigned long from, unsigned long num, int turn_on);
extern pid_t (*WRAP(gettid))(void);

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
extern ssize_t (*WRAP(readahead))(int fd, __off64_t offset, size_t count);
extern int (*WRAP(setxattr)) (const char *path, const char *name, const void *value, size_t size, int flags);
extern int (*WRAP(lsetxattr)) (const char *path, const char *name, const void *value, size_t size, int flags);
extern int (*WRAP(fsetxattr)) (int filedes, const char *name, const void *value, size_t size, int flags);
extern ssize_t (*WRAP(getxattr)) (const char *path, const char *name, void *value, size_t size);
extern ssize_t (*WRAP(lgetxattr)) (const char *path, const char *name, void *value, size_t size);
extern ssize_t (*WRAP(fgetxattr)) (int filedes, const char *name, void *value, size_t size);
extern ssize_t (*WRAP(listxattr)) (const char *path, char *list, size_t size);
extern ssize_t (*WRAP(llistxattr)) (const char *path, char *list, size_t size);
extern ssize_t (*WRAP(flistxattr)) (int filedes, char *list, size_t size);
extern int (*WRAP(removexattr)) (const char *path, const char *name);
extern int (*WRAP(lremovexattr)) (const char *path, const char *name);
extern int (*WRAP(fremovexattr)) (int filedes, const char *name);
extern int (*WRAP(tkill))(int tid, int sig);
extern time_t (*WRAP(time))(time_t *t);
extern int (*WRAP(futex))(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3);
extern int (*WRAP(sched_setaffinity))(__pid_t pid, size_t cpusetsize, const cpu_set_t *mask);
extern int (*WRAP(sched_getaffinity))(__pid_t pid, size_t cpusetsize, cpu_set_t *mask);
extern int (*WRAP(set_thread_area)) (struct user_desc *u_info);
//extern int (*WRAP(io_setup)) (int maxevents, io_context_t *ctxp);
//extern int (*WRAP(io_destroy)) (io_context_t ctx);
//extern long (*WRAP( io_getevents)) (aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout);
//extern long (*WRAP( io_submit))(aio_context_t ctx_id, long nr, struct iocb **iocbpp);
//extern long (*WRAP( io_cancel))(aio_context_t ctx_id, struct iocb *iocb, struct io_event *result);

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
extern int (*WRAP(get_thread_area))(struct user_desc *u_info);
//extern int (*WRAP(lookup_dcookie))(u64 cookie, char * buffer, size_t len);
extern int (*WRAP(epoll_create))(int size);
extern int (*WRAP(epoll_ctl))(int epfd, int op, int fd, struct epoll_event *event);
extern int (*WRAP(epoll_wait))(int epfd, struct epoll_event * events, int maxevents, int timeout);
extern int (*WRAP(remap_file_pages))(void *start, size_t size, int prot, size_t pgoff, int flags);
extern long(*WRAP(sys_set_tid_address)) (int *tidptr);
extern long(*WRAP(sys_restart_syscall))(void);
extern int (*WRAP(semtimedop))(int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout);
extern long (*WRAP(fadvise64_64))(int fs, loff_t offset, loff_t len, int advice);
extern long (*WRAP(sys_timer_create)) (clockid_t which_clock, struct sigevent *timer_event_spec, timer_t *created_timer_id);
extern long (*WRAP(sys_timer_settime)) (timer_t timer_id, int flags, const struct itimerspec *new_setting, struct itimerspec *old_setting);
extern long (*WRAP(sys_timer_gettime)) (timer_t timer_id, struct itimerspec *setting);
extern long (*WRAP(sys_timer_getoverrun)) (timer_t timer_id);
extern long (*WRAP(sys_timer_delete)) (timer_t timer_id);
extern long (*WRAP(sys_clock_settime)) (clockid_t which_clock, const struct timespec *tp);
extern long (*WRAP(sys_clock_gettime)) (clockid_t which_clock, struct timespec *tp);
extern long (*WRAP(sys_clock_getres)) (clockid_t which_clock, struct timespec *tp);
extern long (*WRAP(sys_clock_nanosleep)) (clockid_t which_clock, int flags, const struct timespec *rqtp, struct timespec *rmtp);
extern void (*WRAP(exit_group))(int status);
extern long (*WRAP(sys_tgkill)) (int tgid, int pid, int sig);
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
extern int (*WRAP(utimes))(const char *filename, const struct timeval times[2]);
extern mqd_t (*WRAP(mq_open))(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
extern mqd_t (*WRAP(mq_unlink))(const char *name);
extern mqd_t (*WRAP(mq_timedsend))(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio, const struct timespec *abs_timeout);
extern mqd_t (*WRAP(mq_timedreceive))(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio, const struct timespec *abs_timeout);
extern mqd_t (*WRAP(mq_notify))(mqd_t mqdes, const struct sigevent *notification);
extern mqd_t (*WRAP(mq_getsetattr))(mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr);
extern long (*WRAP(kexec_load))(unsigned long entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long flags);
extern int (*WRAP(waitid))(idtype_t idtype, id_t id, siginfo_t *infop, int options);
//extern key_serial_t (*WRAP(add_key))(const char *type, const char *description, const void *payload, size_t plen, key_serial_t keyring);
//extern key_serial_t (*WRAP(request_key))(const char *type, const char *description, const char *callout_info, key_serial_t keyring);
extern long (*WRAP(keyctl))(int cmd, ...);
extern int (*WRAP(ioprio_get))(int which, int who);
extern int (*WRAP(ioprio_set))(int which, int who, int ioprio);
extern int (*WRAP(inotify_init))(void);
extern int (*WRAP(inotify_add_watch))(int fd, const char *pathname, uint32_t mask);
extern int (*WRAP(inotify_rm_watch))(int fd, uint32_t wd);

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

extern int (*WRAP(openat))(int dirfd, const char *pathname, int flags, mode_t mode);
extern int (*WRAP(mkdirat))(int dirfd, const char *pathname, mode_t mode);
extern int (*WRAP(mknodat))(int dirfd, const char *pathname, mode_t mode, dev_t dev);
extern int (*WRAP(fchownat))(int dirfd, const char *path, uid_t owner, gid_t group, int flags);
extern int (*WRAP(futimesat))(int dirfd, const char *path, const struct timeval times[2]);
extern int (*WRAP(unlinkat))(int dirfd, const char *pathname, int flags);
extern int (*WRAP(renameat))(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
extern int (*WRAP(linkat))(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
extern int (*WRAP(symlinkat))(const char *oldpath, int newdirfd, const char *newpath);
extern ssize_t (*WRAP(readlinkat))(int dirfd, const char *path, char *buf, size_t bufsiz);
extern int (*WRAP(fchmodat))(int dirfd, const char *path, mode_t mode, int flags);
extern int (*WRAP(faccessat))(int dirfd, const char *path, int mode, int flags);
extern int (*WRAP(pselect))(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask);
extern int (*WRAP(ppoll))(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask);
extern int (*WRAP(unshare))(int flags);
extern long (*WRAP(get_robust_list))(int pid, struct robust_list_head **head_ptr, size_t *len_ptr);
extern long (*WRAP(set_robust_list))(struct robust_list_head *head, size_t len);
extern ssize_t (*WRAP(splice))(int fd_in, __off64_t *off_in, int fd_out, __off64_t *off_out, size_t len, unsigned int flags);
extern ssize_t (*WRAP(tee))(int fd_in, int fd_out, size_t len, unsigned int flags);
extern int (*WRAP(sync_file_range))(int fd, __off64_t offset, __off64_t nbytes, unsigned int flags);
extern ssize_t (*WRAP(vmsplice))(int fd, const struct iovec *iov, size_t nr_segs, unsigned int flags);
extern long (*WRAP(move_pages))(pid_t pid, unsigned long nr_pages, const void **address, const int *nodes, int *status, int flags);
/*
#define _SYS_epoll_pwait  281
#define _SYS_signalfd   282
#define _SYS_timerfd_create 283
#define _SYS_eventfd    284
#define _SYS_fallocate    285
#define _SYS_timerfd_settime  286
#define _SYS_timerfd_gettime  287
#define _SYS_accept4    288
#define _SYS_signalfd4    289
#define _SYS_eventfd2   290
#define _SYS_epoll_create1  291
#define _SYS_dup3   292
#define _SYS_pipe2    293
#define _SYS_inotify_init1  294
#define _SYS_preadv   295
#define _SYS_pwritev    296
#define _SYS_rt_tgsigqueueinfo  297
#define _SYS_perf_event_open  298
*/

#endif
