
#include "libfuncs.h"

// libc functions
int (*WRAP(sigwait))(const sigset_t*, int*);
void* (*WRAP(malloc))(size_t);
void  (*WRAP(free))(void *);
void* (*WRAP(realloc))(void *, size_t);
void* (*WRAP(memalign))(size_t, size_t);
size_t (*WRAP(malloc_usable_size))(void *);
int (*WRAP(__clone))(int (*fn)(void *), void *child_stack, int flags, void *arg, pid_t *pid, struct user_desc * tls, pid_t *ctid);

// pthread basics
int (*WRAP(pthread_create))(pthread_t*, const pthread_attr_t*, void *(*)(void*), void*);
int (*WRAP(pthread_cancel))(pthread_t);
int (*WRAP(pthread_detach))(pthread_t);
int (*WRAP(pthread_kill))(pthread_t, int);
int (*WRAP(pthread_join))(pthread_t, void**);
pthread_t (*WRAP(pthread_self))();
int (*WRAP(pthread_exit))(void*);

// pthread mutexes
int (*WRAP(pthread_mutexattr_init))(pthread_mutexattr_t*);
int (*WRAP(pthread_mutex_init))(pthread_mutex_t*, const pthread_mutexattr_t*);
int (*WRAP(pthread_mutex_lock))(pthread_mutex_t*);
int (*WRAP(pthread_mutex_unlock))(pthread_mutex_t*);
int (*WRAP(pthread_mutex_trylock))(pthread_mutex_t*);
int (*WRAP(pthread_mutex_destroy))(pthread_mutex_t*);

// pthread condition variables
int (*WRAP(pthread_condattr_init))(pthread_condattr_t*);
int (*WRAP(pthread_cond_init))(pthread_cond_t*, const pthread_condattr_t*);
int (*WRAP(pthread_cond_wait))(pthread_cond_t*, pthread_mutex_t*);
int (*WRAP(pthread_cond_signal))(pthread_cond_t*);
int (*WRAP(pthread_cond_broadcast))(pthread_cond_t*);
int (*WRAP(pthread_cond_destroy))(pthread_cond_t*);

// pthread barriers
int (*WRAP(pthread_barrier_init))(pthread_barrier_t*, const pthread_barrierattr_t*, unsigned int);
int (*WRAP(pthread_barrier_wait))(pthread_barrier_t*);
int (*WRAP(pthread_barrier_destroy))(pthread_barrier_t*);

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

//#ifdef HANDLE_SYSCALL
#if 1

ssize_t (*WRAP(read))(int, void*, size_t);
ssize_t (*WRAP(write))(int, const void*, size_t);
void* (*WRAP(mmap))(void*, size_t, int, int, int, off_t);
int (*WRAP(open))(const char *pathname, int flags, mode_t mode);
int (*WRAP(close))(int fd);
DIR * (*WRAP(opendir))(const char *name);
int (*WRAP(closedir))(DIR *dir);
FILE* (*WRAP(fopen))(const char*pathname, const char* modes);
FILE* (*WRAP(fopen64))(const char*pathname, const char* modes);
int (*WRAP(fclose))(FILE *fp);
size_t (*WRAP(fread))(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t (*WRAP(fwrite))(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int (*WRAP(stat))(const char *path, struct stat *buf);
int (*WRAP(fstat))(int filedes, struct stat *buf);
int (*WRAP(lstat))(const char *path, struct stat *buf);
int (*WRAP(poll))(struct pollfd *fds, nfds_t nfds, int timeout);
off_t (*WRAP(lseek))(int fildes, off_t offset, int whence);
int (*WRAP(mprotect))(const void *addr, size_t len, int prot);
int (*WRAP(munmap))(void *start, size_t length);

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
int (*WRAP(brk))(void *end_data_segment);
int (*WRAP(sigaction))(int signum, const struct sigaction *act, struct sigaction *oldact);
int (*WRAP(sigprocmask))(int how, const sigset_t *set, sigset_t *oldset);
int (*WRAP(sigreturn))(unsigned long __unused);
int (*WRAP(ioctl))(int d, int request, ...);
ssize_t (*WRAP(pread))(int fd, void *buf, size_t count, off_t offset);
ssize_t (*WRAP(pwrite))(int fd, const void *buf, size_t count, off_t offset);
ssize_t (*WRAP(readv))(int fd, const struct iovec *vector, int count);
ssize_t (*WRAP(writev))(int fd, const struct iovec *vector, int count);
int (*WRAP(access))(const char *pathname, int mode);
int (*WRAP(pipe))(int filedes[2]);
int (*WRAP(select))(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
//int sched_yield(void);
void * (*WRAP(mremap))(void *old_address, size_t old_size , size_t new_size, int flags);
int (*WRAP(msync))(void *start, size_t length, int flags);
int (*WRAP(mincore))(void *start, size_t length, unsigned char *vec);
int (*WRAP(madvise))(void *start, size_t length, int advice);
int (*WRAP(shmget))(key_t key, size_t size, int shmflg);
void *(*WRAP(shmat))(int shmid, const void *shmaddr, int shmflg);
int (*WRAP(shmctl))(int shmid, int cmd, struct shmid_ds *buf);

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
int (*WRAP(dup))(int oldfd);
int (*WRAP(dup2))(int oldfd, int newfd);
int (*WRAP(pause))(void); // FIXME
int (*WRAP(nanosleep))(const struct timespec *req, struct timespec *rem); //FIXME
int (*WRAP(getitimer))(int which, struct itimerval *value);
unsigned int (*WRAP(alarm))(unsigned int seconds); //FIXME
int (*WRAP(setitimer))(int which, const struct itimerval *value, struct itimerval *ovalue);
//pid_t getpid(void); //FIXME
ssize_t (*WRAP(sendfile))(int out_fd, int in_fd, off_t *offset, size_t count);
int (*WRAP(socket))(int domain, int type, int protocol);
int (*WRAP(connect))(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
int (*WRAP(accept))(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t  (*WRAP(sendto))(int  s,  const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t (*WRAP(recvfrom))(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
ssize_t (*WRAP(sendmsg))(int s, const struct msghdr *msg, int flags);
ssize_t (*WRAP(recvmsg))(int s, struct msghdr *msg, int flags);
int (*WRAP(shutdown))(int s, int how); //FIXME
int (*WRAP(bind))(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);
int (*WRAP(listen))(int sockfd, int backlog);
int (*WRAP(getsockname))(int s, struct sockaddr *name, socklen_t *namelen);
int (*WRAP(getpeername))(int s, struct sockaddr *name, socklen_t *namelen);
int (*WRAP(socketpair))(int d, int type, int protocol, int sv[2]);
int (*WRAP(setsockopt))(int s, int level, int optname, const void *optval, socklen_t optlen);
int (*WRAP(getsockopt))(int s, int level, int optname, void *optval, socklen_t *optlen);
int (*WRAP(clone))(int (*fn)(void *), void *child_stack, int flags, void *arg, ... /* pid_t *pid, struct user_desc *tls, pid_t *ctid */ );
pid_t (*WRAP(fork))(void);
pid_t (*WRAP(vfork))(void);
int (*WRAP(execve))(const char *filename, char *const argv[],
           char *const envp[]);
void (*WRAP(exit))(int status);
pid_t (*WRAP(wait4))(pid_t pid, void *status, int options, struct rusage *rusage);
int (*WRAP(kill))(pid_t pid, int sig);
int (*WRAP(uname))(struct utsname *buf);

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
int (*WRAP(semget))(key_t key, int nsems, int semflg);
int (*WRAP(semop))(int semid, struct sembuf *sops, size_t nsops);
int (*WRAP(semctl))(int semid, int semnum, int cmd, ...);
int (*WRAP(fcntl))(int fd, int cmd, long arg);
int (*WRAP(flock))(int fd, int operation);
int (*WRAP(fsync))(int fd);
int (*WRAP(fdatasync))(int fd);
int (*WRAP(truncate))(const char *path, off_t length);
int (*WRAP(ftruncate))(int fd, off_t length);
int (*WRAP(getdents))(unsigned int fd, struct dirent *dirp, unsigned int count);
char * (*WRAP(getcwd))(char *buf, unsigned long size);
int (*WRAP(chdir))(const char *path);
int (*WRAP(fchdir))(int fd);

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
int (*WRAP(rename))(const char *oldpath, const char *newpath);
int (*WRAP(mkdir))(const char *pathname, mode_t mode);
int (*WRAP(rmdir))(const char *pathname);
int (*WRAP(creat))(const char *pathname, mode_t mode);
int (*WRAP(link))(const char *oldpath, const char *newpath);
int (*WRAP(unlink))(const char *pathname);
int (*WRAP(symlink))(const char *oldpath, const char *newpath);
ssize_t (*WRAP(readlink))(const char *path, char *buf, size_t bufsiz);
int (*WRAP(chmod))(const char *path, mode_t mode);
int (*WRAP(fchmod))(int fildes, mode_t mode);
int (*WRAP(chown))(const char *path, uid_t owner, gid_t group);
int (*WRAP(fchown))(int fd, uid_t owner, gid_t group);
int (*WRAP(lchown))(const char *path, uid_t owner, gid_t group);
mode_t (*WRAP(umask))(mode_t mask);
int (*WRAP(gettimeofday))(struct timeval *tv, struct timezone *tz);
int (*WRAP(getrlimit))(int resource, struct rlimit *rlim);
int (*WRAP(getrusage))(int who, struct rusage *usage);
int (*WRAP(sysinfo))(struct sysinfo *info);
clock_t (*WRAP(times))(struct tms *buf);
//long (*WRAP(ptrace))(enum __ptrace_request request, pid_t pid, void *addr, void *data); //FIXME
uid_t (*WRAP(getuid))(void);
int (*WRAP(syslog))(int type, char *bufp, int len);

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
*/

gid_t (*WRAP(getgid))(void);
int (*WRAP(setuid))(uid_t uid);
int (*WRAP(setgid))(gid_t gid);
uid_t (*WRAP(geteuid))(void);
gid_t (*WRAP(getegid))(void);
int (*WRAP(setpgid))(pid_t pid, pid_t pgid);
pid_t (*WRAP(getppid))(void);
pid_t (*WRAP(getpgrp))(void);
pid_t (*WRAP(setsid))(void);
int (*WRAP(setreuid))(uid_t ruid, uid_t euid);
int (*WRAP(setregid))(gid_t rgid, gid_t egid);
int (*WRAP(getgroups))(int size, gid_t list[]);
int (*WRAP(setgroups))(size_t size, const gid_t *list);
int (*WRAP(setresuid))(uid_t ruid, uid_t euid, uid_t suid);
int (*WRAP(getresuid))(uid_t *ruid, uid_t *euid, uid_t *suid);
int (*WRAP(setresgid))(gid_t rgid, gid_t egid, gid_t sgid);
int (*WRAP(getresgid))(gid_t *rgid, gid_t *egid, gid_t *sgid);
pid_t (*WRAP(getpgid))(pid_t pid);
int (*WRAP(setfsuid))(uid_t fsuid);
int (*WRAP(setfsgid))(uid_t fsgid);

/*
#define _SYS_getsid                            124
#define _SYS_capget                            125
#define _SYS_capset                            126
#define _SYS_rt_sigpending                     127
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

pid_t (*WRAP(getsid))(pid_t pid);
int (*WRAP(sigpending))(sigset_t *set);
int (*WRAP(sigtimedwait))(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
int (*WRAP(sigsuspend))(const sigset_t *mask);
int (*WRAP(sigaltstack))(const stack_t *ss, stack_t *oss);
int (*WRAP(utime))(const char *filename, const struct utimbuf *buf);
int (*WRAP(mknod))(const char *pathname, mode_t mode, dev_t dev);
int (*WRAP(uselib))(const char *library);
int (*WRAP(personality))(unsigned long persona);
int (*WRAP(ustat))(dev_t dev, struct ustat *ubuf);
int (*WRAP(statfs))(const char *path, struct statfs *buf);
int (*WRAP(fstatfs))(int fd, struct statfs *buf);
int (*WRAP(sysfs))(int option, unsigned int fs_index, char *buf);
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
int (*WRAP(getpriority))(int which, int who);
int (*WRAP(setpriority))(int which, int who, int prio);
int (*WRAP(sched_setparam))(pid_t pid, const struct sched_param *param);
int (*WRAP(sched_getparam))(pid_t pid, struct sched_param *param);
int (*WRAP(sched_setscheduler))(pid_t pid, int policy, const struct sched_param *param);
int (*WRAP(sched_getscheduler))(pid_t pid);
int (*WRAP(sched_get_priority_max))(int policy);
int (*WRAP(sched_get_priority_min))(int policy);
int (*WRAP(sched_rr_get_interval))(pid_t pid, struct timespec *tp);
int (*WRAP(mlock))(const void *addr, size_t len);
int (*WRAP(munlock))(const void *addr, size_t len);
int (*WRAP(mlockall))(int flags);
int (*WRAP(munlockall))(void);
int (*WRAP(vhangup))(void);
int (*WRAP(modify_ldt))(int func, void *ptr, unsigned long bytecount);
int (*WRAP(pivot_root))(const char *new_root, const char *put_old);
int (*WRAP(_sysctl))(struct __sysctl_args *args);
int (*WRAP( prctl))(int  option,  unsigned  long arg2, unsigned long arg3 , unsigned long arg4, unsigned long arg5);
int (*WRAP(arch_prctl))(int code, unsigned long addr);
int (*WRAP(adjtimex))(struct timex *buf);
int (*WRAP(setrlimit))(int resource, const struct rlimit *rlim);
int (*WRAP(chroot))(const char *path);
void(*WRAP( sync))(void);
int (*WRAP(acct))(const char *filename);


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
*/
int (*WRAP(settimeofday))(const struct timeval *tv , const struct timezone *tz);
int (*WRAP(mount))(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data);
int (*WRAP(umount2))(const char *target, int flags);
int (*WRAP(swapon))(const char *path, int swapflags);
int (*WRAP(swapoff))(const char *path);
int (*WRAP(reboot))(int magic, int magic2, int flag, void *arg);
int (*WRAP(sethostname))(const char *name, size_t len);
int (*WRAP(setdomainname))(const char *name, size_t len);
int (*WRAP(iopl))(int level);
int (*WRAP(ioperm))(unsigned long from, unsigned long num, int turn_on);
pid_t (*WRAP(gettid))(void);

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
ssize_t (*WRAP(readahead))(int fd, __off64_t offset, size_t count);
int (*WRAP(setxattr)) (const char *path, const char *name, const void *value, size_t size, int flags);
int (*WRAP(lsetxattr)) (const char *path, const char *name, const void *value, size_t size, int flags);
int (*WRAP(fsetxattr)) (int filedes, const char *name, const void *value, size_t size, int flags);
ssize_t (*WRAP(getxattr)) (const char *path, const char *name, void *value, size_t size);
ssize_t (*WRAP(lgetxattr)) (const char *path, const char *name, void *value, size_t size);
ssize_t (*WRAP(fgetxattr)) (int filedes, const char *name, void *value, size_t size);
ssize_t (*WRAP(listxattr)) (const char *path, char *list, size_t size);
ssize_t (*WRAP(llistxattr)) (const char *path, char *list, size_t size);
ssize_t (*WRAP(flistxattr)) (int filedes, char *list, size_t size);
int (*WRAP(removexattr)) (const char *path, const char *name);
int (*WRAP(lremovexattr)) (const char *path, const char *name);
int (*WRAP(fremovexattr)) (int filedes, const char *name);
int (*WRAP(tkill))(int tid, int sig);
time_t (*WRAP(time))(time_t *t);
int (*WRAP(futex))(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3);
int (*WRAP(sched_setaffinity))(__pid_t pid, size_t cpusetsize, const cpu_set_t *mask);
int (*WRAP(sched_getaffinity))(__pid_t pid, size_t cpusetsize, cpu_set_t *mask);
int (*WRAP(set_thread_area)) (struct user_desc *u_info);
//int (*WRAP(io_setup)) (int maxevents, io_context_t *ctxp);
//int (*WRAP(io_destroy)) (io_context_t ctx);
//long (*WRAP( io_getevents)) (aio_context_t ctx_id, long min_nr, long nr, struct io_event *events, struct timespec *timeout);
//long (*WRAP( io_submit))(aio_context_t ctx_id, long nr, struct iocb **iocbpp);
//long (*WRAP( io_cancel))(aio_context_t ctx_id, struct iocb *iocb, struct io_event *result);

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
int (*WRAP(get_thread_area))(struct user_desc *u_info);
//int (*WRAP(lookup_dcookie))(u64 cookie, char * buffer, size_t len);
int (*WRAP(epoll_create))(int size);
int (*WRAP(epoll_ctl))(int epfd, int op, int fd, struct epoll_event *event);
int (*WRAP(epoll_wait))(int epfd, struct epoll_event * events, int maxevents, int timeout);
int (*WRAP(remap_file_pages))(void *start, size_t size, int prot, size_t pgoff, int flags);
long(*WRAP( sys_set_tid_address)) (int *tidptr);
long(*WRAP( sys_restart_syscall))(void);
int (*WRAP( semtimedop))(int semid, struct sembuf *sops, size_t nsops, const struct timespec *timeout);
long (*WRAP(fadvise64_64))(int fs, loff_t offset, loff_t len, int advice);
long (*WRAP(sys_timer_create)) (clockid_t which_clock, struct sigevent *timer_event_spec, timer_t *created_timer_id);
long (*WRAP(sys_timer_settime)) (timer_t timer_id, int flags, const struct itimerspec *new_setting, struct itimerspec *old_setting);
long (*WRAP(sys_timer_gettime)) (timer_t timer_id, struct itimerspec *setting);
long (*WRAP(sys_timer_getoverrun)) (timer_t timer_id);
long (*WRAP(sys_timer_delete)) (timer_t timer_id);
long (*WRAP(sys_clock_settime)) (clockid_t which_clock, const struct timespec *tp);
long (*WRAP(sys_clock_gettime)) (clockid_t which_clock, struct timespec *tp);
long (*WRAP(sys_clock_getres)) (clockid_t which_clock, struct timespec *tp);
long (*WRAP(sys_clock_nanosleep)) (clockid_t which_clock, int flags, const struct timespec *rqtp, struct timespec *rmtp);
void (*WRAP(exit_group))(int status);
long (*WRAP(sys_tgkill)) (int tgid, int pid, int sig);

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
*/
int (*WRAP(utimes))(const char *filename, const struct timeval times[2]);
mqd_t (*WRAP(mq_open))(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
mqd_t (*WRAP(mq_unlink))(const char *name);
mqd_t (*WRAP(mq_timedsend))(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned msg_prio, const struct timespec *abs_timeout);
mqd_t (*WRAP(mq_timedreceive))(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned *msg_prio, const struct timespec *abs_timeout);
mqd_t (*WRAP(mq_notify))(mqd_t mqdes, const struct sigevent *notification);
mqd_t (*WRAP(mq_getsetattr))(mqd_t mqdes, struct mq_attr *newattr, struct mq_attr *oldattr);
long (*WRAP(kexec_load))(unsigned long entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long flags);
int (*WRAP(waitid))(idtype_t idtype, id_t id, siginfo_t *infop, int options);
key_serial_t (*WRAP(add_key))(const char *type, const char *description, const void *payload, size_t plen, key_serial_t keyring);
key_serial_t (*WRAP(request_key))(const char *type, const char *description, const char *callout_info, key_serial_t keyring);
long (*WRAP(keyctl))(int cmd, ...);
int (*WRAP(ioprio_get))(int which, int who);
int (*WRAP(ioprio_set))(int which, int who, int ioprio);
int (*WRAP(inotify_init))(void);
int (*WRAP(inotify_add_watch))(int fd, const char *pathname, uint32_t mask);
int (*WRAP(inotify_rm_watch))(int fd, uint32_t wd);

/*
#define _SYS_migrate_pages  256
#define _SYS_openat   257
#define _SYS_mkdirat    258
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



//int (*WRAP(openat))(int dirfd, const char *pathname, int flags);
int (*WRAP(openat))(int dirfd, const char *pathname, int flags, mode_t mode);
int (*WRAP(mkdirat))(int dirfd, const char *pathname, mode_t mode);
int (*WRAP(mknodat))(int dirfd, const char *pathname, mode_t mode, dev_t dev);
int (*WRAP(fchownat))(int dirfd, const char *path, uid_t owner, gid_t group, int flags);
int (*WRAP(futimesat))(int dirfd, const char *path, const struct timeval times[2]);
int (*WRAP(unlinkat))(int dirfd, const char *pathname, int flags);
int (*WRAP(renameat))(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
int (*WRAP(linkat))(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
int (*WRAP(symlinkat))(const char *oldpath, int newdirfd, const char *newpath);
ssize_t (*WRAP(readlinkat))(int dirfd, const char *path, char *buf, size_t bufsiz);
int (*WRAP(fchmodat))(int dirfd, const char *path, mode_t mode, int flags);
int (*WRAP(faccessat))(int dirfd, const char *path, int mode, int flags);
int (*WRAP(pselect))(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t *sigmask);
int (*WRAP(ppoll))(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout, const sigset_t *sigmask);
int (*WRAP(unshare))(int flags);
long (*WRAP(get_robust_list))(int pid, struct robust_list_head **head_ptr, size_t *len_ptr);
long (*WRAP(set_robust_list))(struct robust_list_head *head, size_t len);
int (*WRAP(splice))(int fd_in, __off64_t *off_in, int fd_out, __off64_t *off_out, size_t len, unsigned int flags);
int (*WRAP(tee))(int fd_in, int fd_out, size_t len, unsigned int flags);
int (*WRAP(sync_file_range))(int fd, off64_t offset, off64_t nbytes, unsigned int flags);
int (*WRAP(vmsplice))(int fd, const struct iovec *iov, size_t nr_segs, unsigned int flags);
long (*WRAP(move_pages))(pid_t pid, unsigned long nr_pages, const void **address, const int *nodes, int *status, int flags);
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
#define _SYS_recvmmsg   299
*/
#endif

#ifndef assert
#define assert(x) 
#endif

#define SET_WRAPPED(x, handle) WRAP(x) = (typeof(WRAP(x)))dlsym(handle, #x); assert(#x)

void init_real_functions() {

	SET_WRAPPED(mmap, RTLD_NEXT);
	SET_WRAPPED(malloc, RTLD_NEXT);
	SET_WRAPPED(free, RTLD_NEXT);
	SET_WRAPPED(realloc, RTLD_NEXT);
	SET_WRAPPED(memalign, RTLD_NEXT);
	SET_WRAPPED(malloc_usable_size, RTLD_NEXT);
	SET_WRAPPED(read, RTLD_NEXT);
	SET_WRAPPED(write, RTLD_NEXT);
	SET_WRAPPED(sigwait, RTLD_NEXT);
	SET_WRAPPED(time, RTLD_NEXT);
	SET_WRAPPED(gettimeofday, RTLD_NEXT);

//#ifdef HANDLE_SYSCALL
  SET_WRAPPED(read, RTLD_NEXT);
  SET_WRAPPED(write, RTLD_NEXT);
  SET_WRAPPED(mmap, RTLD_NEXT);
  SET_WRAPPED(open, RTLD_NEXT);
  SET_WRAPPED(close, RTLD_NEXT);
  //fprintf(stderr, "fopen is %p fopen64 is %p\n", WRAP(fopen), WRAP(fopen64));
  SET_WRAPPED(opendir, RTLD_NEXT);
  SET_WRAPPED(closedir, RTLD_NEXT);
  SET_WRAPPED(fopen, RTLD_NEXT);
  SET_WRAPPED(fopen64, RTLD_NEXT);
  SET_WRAPPED(fclose, RTLD_NEXT);
 // fprintf(stderr, "fopen is %p fopen64 is %p fclose is %p\n", WRAP(fopen), WRAP(fopen64), WRAP(fclose));
  SET_WRAPPED(fread, RTLD_NEXT);
  SET_WRAPPED(fwrite, RTLD_NEXT);
  SET_WRAPPED(stat, RTLD_NEXT);
  SET_WRAPPED(fstat, RTLD_NEXT);
  SET_WRAPPED(lstat, RTLD_NEXT);
  SET_WRAPPED(poll, RTLD_NEXT);
  SET_WRAPPED(lseek, RTLD_NEXT);
  SET_WRAPPED(mprotect, RTLD_NEXT);
  SET_WRAPPED(munmap, RTLD_NEXT);
  SET_WRAPPED(brk, RTLD_NEXT);
  SET_WRAPPED(sigaction, RTLD_NEXT);
  SET_WRAPPED(sigprocmask, RTLD_NEXT);
  SET_WRAPPED(sigreturn, RTLD_NEXT);
  SET_WRAPPED(ioctl, RTLD_NEXT);
  SET_WRAPPED(pread, RTLD_NEXT);
  SET_WRAPPED(pwrite, RTLD_NEXT);
  SET_WRAPPED(readv, RTLD_NEXT);
  SET_WRAPPED(writev, RTLD_NEXT);
  SET_WRAPPED(access, RTLD_NEXT);
  SET_WRAPPED(pipe, RTLD_NEXT);
  SET_WRAPPED(select, RTLD_NEXT);
//  SET_WRAPPED(sched_yield, RTLD_NEXT);
  SET_WRAPPED(mremap, RTLD_NEXT);
  SET_WRAPPED(msync, RTLD_NEXT);
  SET_WRAPPED(mincore, RTLD_NEXT);
  SET_WRAPPED(madvise, RTLD_NEXT);
  SET_WRAPPED(shmget, RTLD_NEXT);
  SET_WRAPPED(shmat, RTLD_NEXT);
  SET_WRAPPED(shmctl, RTLD_NEXT);
  SET_WRAPPED(dup, RTLD_NEXT);
  SET_WRAPPED(dup2, RTLD_NEXT);
  SET_WRAPPED(pause, RTLD_NEXT);
  SET_WRAPPED(nanosleep, RTLD_NEXT);
  SET_WRAPPED(getitimer, RTLD_NEXT);
  SET_WRAPPED(alarm, RTLD_NEXT);
  SET_WRAPPED(setitimer, RTLD_NEXT);
//  SET_WRAPPED(getpid, RTLD_NEXT);
  SET_WRAPPED(sendfile, RTLD_NEXT);
  SET_WRAPPED(socket, RTLD_NEXT);
  SET_WRAPPED(connect, RTLD_NEXT);
  SET_WRAPPED(accept, RTLD_NEXT);
  SET_WRAPPED(sendto, RTLD_NEXT);
  SET_WRAPPED(recvfrom, RTLD_NEXT);
  SET_WRAPPED(sendmsg, RTLD_NEXT);
  SET_WRAPPED(recvmsg, RTLD_NEXT);
  SET_WRAPPED(shutdown, RTLD_NEXT);
  SET_WRAPPED(bind, RTLD_NEXT);
  SET_WRAPPED(listen, RTLD_NEXT);
  SET_WRAPPED(getsockname, RTLD_NEXT);
  SET_WRAPPED(getpeername, RTLD_NEXT);
  SET_WRAPPED(socketpair, RTLD_NEXT);
  SET_WRAPPED(setsockopt, RTLD_NEXT);
  SET_WRAPPED(getsockopt, RTLD_NEXT);
  SET_WRAPPED(clone, RTLD_NEXT);
  SET_WRAPPED(fork, RTLD_NEXT);
  SET_WRAPPED(vfork, RTLD_NEXT);
  SET_WRAPPED(execve, RTLD_NEXT);
  SET_WRAPPED(exit, RTLD_NEXT);
  SET_WRAPPED(wait4, RTLD_NEXT);
  SET_WRAPPED(kill, RTLD_NEXT);
  SET_WRAPPED(uname, RTLD_NEXT);
  SET_WRAPPED(semget, RTLD_NEXT);
  SET_WRAPPED(semop, RTLD_NEXT);
  SET_WRAPPED(semctl, RTLD_NEXT);
  SET_WRAPPED(fcntl, RTLD_NEXT);
  SET_WRAPPED(flock, RTLD_NEXT);
  SET_WRAPPED(fsync, RTLD_NEXT);
  SET_WRAPPED(fdatasync, RTLD_NEXT);
  SET_WRAPPED(truncate, RTLD_NEXT);
  SET_WRAPPED(ftruncate, RTLD_NEXT);
  SET_WRAPPED(getdents, RTLD_NEXT);
  SET_WRAPPED(getcwd, RTLD_NEXT);
  SET_WRAPPED(chdir, RTLD_NEXT);
  SET_WRAPPED(fchdir, RTLD_NEXT);
  SET_WRAPPED(rename, RTLD_NEXT);
  SET_WRAPPED(mkdir, RTLD_NEXT);
  SET_WRAPPED(rmdir, RTLD_NEXT);
  SET_WRAPPED(creat, RTLD_NEXT);
  SET_WRAPPED(link, RTLD_NEXT);
  SET_WRAPPED(unlink, RTLD_NEXT);
  SET_WRAPPED(symlink, RTLD_NEXT);
  SET_WRAPPED(readlink, RTLD_NEXT);
  SET_WRAPPED(chmod, RTLD_NEXT);
  SET_WRAPPED(fchmod, RTLD_NEXT);
  SET_WRAPPED(chown, RTLD_NEXT);
  SET_WRAPPED(fchown, RTLD_NEXT);
  SET_WRAPPED(lchown, RTLD_NEXT);
  SET_WRAPPED(umask, RTLD_NEXT);
  SET_WRAPPED(gettimeofday, RTLD_NEXT);
  SET_WRAPPED(getrlimit, RTLD_NEXT);
  SET_WRAPPED(getrusage, RTLD_NEXT);
  SET_WRAPPED(sysinfo, RTLD_NEXT);
  SET_WRAPPED(times, RTLD_NEXT);
//  SET_WRAPPED(ptrace, RTLD_NEXT);
  SET_WRAPPED(getuid, RTLD_NEXT);
  SET_WRAPPED(syslog, RTLD_NEXT);
  SET_WRAPPED(getgid, RTLD_NEXT);
  SET_WRAPPED(setuid, RTLD_NEXT);
  SET_WRAPPED(setgid, RTLD_NEXT);
  SET_WRAPPED(geteuid, RTLD_NEXT);
  SET_WRAPPED(getegid, RTLD_NEXT);
  SET_WRAPPED(setpgid, RTLD_NEXT);
  SET_WRAPPED(getppid, RTLD_NEXT);
  SET_WRAPPED(getpgrp, RTLD_NEXT);
  SET_WRAPPED(setsid, RTLD_NEXT);
  SET_WRAPPED(setreuid, RTLD_NEXT);
  SET_WRAPPED(setregid, RTLD_NEXT);
  SET_WRAPPED(getgroups, RTLD_NEXT);
  SET_WRAPPED(setgroups, RTLD_NEXT);
  SET_WRAPPED(setresuid, RTLD_NEXT);
  SET_WRAPPED(getresuid, RTLD_NEXT);
  SET_WRAPPED(setresgid, RTLD_NEXT);
  SET_WRAPPED(getresgid, RTLD_NEXT);
  SET_WRAPPED(getpgid, RTLD_NEXT);
  SET_WRAPPED(setfsuid, RTLD_NEXT);
  SET_WRAPPED(setfsgid, RTLD_NEXT);
  SET_WRAPPED(getsid, RTLD_NEXT);
  SET_WRAPPED(sigpending, RTLD_NEXT);
  SET_WRAPPED(sigtimedwait, RTLD_NEXT);
  SET_WRAPPED(sigsuspend, RTLD_NEXT);
  SET_WRAPPED(sigaltstack, RTLD_NEXT);
  SET_WRAPPED(utime, RTLD_NEXT);
  SET_WRAPPED(mknod, RTLD_NEXT);
  SET_WRAPPED(uselib, RTLD_NEXT);
  SET_WRAPPED(personality, RTLD_NEXT);
  SET_WRAPPED(ustat, RTLD_NEXT);
  SET_WRAPPED(statfs, RTLD_NEXT);
  SET_WRAPPED(fstatfs, RTLD_NEXT);
  SET_WRAPPED(sysfs, RTLD_NEXT);
  SET_WRAPPED(getpriority, RTLD_NEXT);
  SET_WRAPPED(setpriority, RTLD_NEXT);
  SET_WRAPPED(sched_setparam, RTLD_NEXT);
  SET_WRAPPED(sched_getparam, RTLD_NEXT);
  SET_WRAPPED(sched_setscheduler, RTLD_NEXT);
  SET_WRAPPED(sched_getscheduler, RTLD_NEXT);
  SET_WRAPPED(sched_get_priority_max, RTLD_NEXT);
  SET_WRAPPED(sched_get_priority_min, RTLD_NEXT);
  SET_WRAPPED(sched_rr_get_interval, RTLD_NEXT);
  SET_WRAPPED(mlock, RTLD_NEXT);
  SET_WRAPPED(munlock, RTLD_NEXT);
  SET_WRAPPED(mlockall, RTLD_NEXT);
  SET_WRAPPED(munlockall, RTLD_NEXT);
  SET_WRAPPED(vhangup, RTLD_NEXT);
  SET_WRAPPED(modify_ldt, RTLD_NEXT);
  SET_WRAPPED(pivot_root, RTLD_NEXT);
  SET_WRAPPED(_sysctl, RTLD_NEXT);
  SET_WRAPPED( prctl, RTLD_NEXT);
  SET_WRAPPED(arch_prctl, RTLD_NEXT);
  SET_WRAPPED(adjtimex, RTLD_NEXT);
  SET_WRAPPED(setrlimit, RTLD_NEXT);
  SET_WRAPPED(chroot, RTLD_NEXT);
  SET_WRAPPED(sync, RTLD_NEXT);
  SET_WRAPPED(acct, RTLD_NEXT);
  SET_WRAPPED(settimeofday, RTLD_NEXT);
  SET_WRAPPED(mount, RTLD_NEXT);
  SET_WRAPPED(umount2, RTLD_NEXT);
  SET_WRAPPED(swapon, RTLD_NEXT);
  SET_WRAPPED(swapoff, RTLD_NEXT);
  SET_WRAPPED(reboot, RTLD_NEXT);
  SET_WRAPPED(sethostname, RTLD_NEXT);
  SET_WRAPPED(setdomainname, RTLD_NEXT);
  SET_WRAPPED(iopl, RTLD_NEXT);
  SET_WRAPPED(ioperm, RTLD_NEXT);
  SET_WRAPPED(gettid, RTLD_NEXT);
  SET_WRAPPED(readahead, RTLD_NEXT);
  SET_WRAPPED(setxattr, RTLD_NEXT);
  SET_WRAPPED(lsetxattr, RTLD_NEXT);
  SET_WRAPPED(fsetxattr, RTLD_NEXT);
  SET_WRAPPED(getxattr, RTLD_NEXT);
  SET_WRAPPED(lgetxattr, RTLD_NEXT);
  SET_WRAPPED(fgetxattr, RTLD_NEXT);
  SET_WRAPPED(listxattr, RTLD_NEXT);
  SET_WRAPPED(llistxattr, RTLD_NEXT);
  SET_WRAPPED(flistxattr, RTLD_NEXT);
  SET_WRAPPED(removexattr, RTLD_NEXT);
  SET_WRAPPED(lremovexattr, RTLD_NEXT);
  SET_WRAPPED(fremovexattr, RTLD_NEXT);
  SET_WRAPPED(tkill, RTLD_NEXT);
  SET_WRAPPED(time, RTLD_NEXT);
  SET_WRAPPED(futex, RTLD_NEXT);
  SET_WRAPPED(sched_setaffinity, RTLD_NEXT);
  SET_WRAPPED(sched_getaffinity, RTLD_NEXT);
  SET_WRAPPED(set_thread_area, RTLD_NEXT);
//  SET_WRAPPED(io_setup, RTLD_NEXT);
//  SET_WRAPPED(io_destroy, RTLD_NEXT);
//  SET_WRAPPED( io_getevents, RTLD_NEXT);
//  SET_WRAPPED( io_submit, RTLD_NEXT);
//  SET_WRAPPED( io_cancel, RTLD_NEXT);
  SET_WRAPPED(get_thread_area, RTLD_NEXT);
//  SET_WRAPPED(lookup_dcookie, RTLD_NEXT);
  SET_WRAPPED(epoll_create, RTLD_NEXT);
  SET_WRAPPED(epoll_ctl, RTLD_NEXT);
  SET_WRAPPED(epoll_wait, RTLD_NEXT);
  SET_WRAPPED(remap_file_pages, RTLD_NEXT);
  SET_WRAPPED( sys_set_tid_address, RTLD_NEXT);
  SET_WRAPPED( sys_restart_syscall, RTLD_NEXT);
  SET_WRAPPED( semtimedop, RTLD_NEXT);
  SET_WRAPPED(fadvise64_64, RTLD_NEXT);
  SET_WRAPPED(sys_timer_create, RTLD_NEXT);
  SET_WRAPPED(sys_timer_settime, RTLD_NEXT);
  SET_WRAPPED(sys_timer_gettime, RTLD_NEXT);
  SET_WRAPPED(sys_timer_getoverrun, RTLD_NEXT);
  SET_WRAPPED(sys_timer_delete, RTLD_NEXT);
  SET_WRAPPED(sys_clock_settime, RTLD_NEXT);
  SET_WRAPPED(sys_clock_gettime, RTLD_NEXT);
  SET_WRAPPED(sys_clock_getres, RTLD_NEXT);
  SET_WRAPPED(sys_clock_nanosleep, RTLD_NEXT);
  SET_WRAPPED(exit_group, RTLD_NEXT);
  SET_WRAPPED(sys_tgkill, RTLD_NEXT);
  SET_WRAPPED(utimes, RTLD_NEXT);
  SET_WRAPPED(mq_open, RTLD_NEXT);
  SET_WRAPPED(mq_unlink, RTLD_NEXT);
  SET_WRAPPED(mq_timedsend, RTLD_NEXT);
  SET_WRAPPED(mq_timedreceive, RTLD_NEXT);
  SET_WRAPPED(mq_notify, RTLD_NEXT);
  SET_WRAPPED(mq_getsetattr, RTLD_NEXT);
  SET_WRAPPED(kexec_load, RTLD_NEXT);
  SET_WRAPPED(waitid, RTLD_NEXT);
  SET_WRAPPED(add_key, RTLD_NEXT);
  SET_WRAPPED(request_key, RTLD_NEXT);
  SET_WRAPPED(keyctl, RTLD_NEXT);
  SET_WRAPPED(ioprio_get, RTLD_NEXT);
  SET_WRAPPED(ioprio_set, RTLD_NEXT);
  SET_WRAPPED(inotify_init, RTLD_NEXT);
  SET_WRAPPED(inotify_add_watch, RTLD_NEXT);
  SET_WRAPPED(inotify_rm_watch, RTLD_NEXT);
  SET_WRAPPED(openat, RTLD_NEXT);
  SET_WRAPPED(mkdirat, RTLD_NEXT);
  SET_WRAPPED(mknodat, RTLD_NEXT);
  SET_WRAPPED(fchownat, RTLD_NEXT);
  SET_WRAPPED(futimesat, RTLD_NEXT);
  SET_WRAPPED(unlinkat, RTLD_NEXT);
  SET_WRAPPED(renameat, RTLD_NEXT);
  SET_WRAPPED(linkat, RTLD_NEXT);
  SET_WRAPPED(symlinkat, RTLD_NEXT);
  SET_WRAPPED(readlinkat, RTLD_NEXT);
  SET_WRAPPED(fchmodat, RTLD_NEXT);
  SET_WRAPPED(faccessat, RTLD_NEXT);
  SET_WRAPPED(pselect, RTLD_NEXT);
  SET_WRAPPED(ppoll, RTLD_NEXT);
  SET_WRAPPED(unshare, RTLD_NEXT);
  SET_WRAPPED(get_robust_list, RTLD_NEXT);
  SET_WRAPPED(set_robust_list, RTLD_NEXT);
  SET_WRAPPED(splice, RTLD_NEXT);
  SET_WRAPPED(tee, RTLD_NEXT);
  SET_WRAPPED(sync_file_range, RTLD_NEXT);
  SET_WRAPPED(vmsplice, RTLD_NEXT);
  SET_WRAPPED(move_pages, RTLD_NEXT);
	SET_WRAPPED(__clone, RTLD_NEXT);

//  fprintf(stderr, "__clone function is at %p\n", WRAP(__clone));
//#endif

	void *pthread_handle = dlopen("libpthread.so.0", RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD);
	if (pthread_handle == NULL) {
		fprintf(stderr, "Unable to load libpthread.so.0\n");
		_exit(2);
	}

	SET_WRAPPED(pthread_create, pthread_handle);
	SET_WRAPPED(pthread_cancel, pthread_handle);
	SET_WRAPPED(pthread_detach, pthread_handle);
	SET_WRAPPED(pthread_kill, pthread_handle);
	SET_WRAPPED(pthread_join, pthread_handle);
	SET_WRAPPED(pthread_self, pthread_handle);
	SET_WRAPPED(pthread_exit, pthread_handle);

	SET_WRAPPED(pthread_mutex_init, pthread_handle);
	SET_WRAPPED(pthread_mutex_lock, pthread_handle);
	SET_WRAPPED(pthread_mutex_unlock, pthread_handle);
	SET_WRAPPED(pthread_mutex_trylock, pthread_handle);
	SET_WRAPPED(pthread_mutex_destroy, pthread_handle);
	SET_WRAPPED(pthread_mutexattr_init, pthread_handle);

	SET_WRAPPED(pthread_condattr_init, pthread_handle);
	SET_WRAPPED(pthread_cond_init, pthread_handle);
	SET_WRAPPED(pthread_cond_wait, pthread_handle);
	SET_WRAPPED(pthread_cond_signal, pthread_handle);
	SET_WRAPPED(pthread_cond_broadcast, pthread_handle);
	SET_WRAPPED(pthread_cond_destroy, pthread_handle);

	SET_WRAPPED(pthread_barrier_init, pthread_handle);
	SET_WRAPPED(pthread_barrier_wait, pthread_handle);
	SET_WRAPPED(pthread_barrier_destroy, pthread_handle);

}
