#if !defined(DOUBLETAKE_RECORD_H)
#define DOUBLETAKE_RECORD_H

/*
 * @file   recordsyscalls.h
 * @brief  A simple list (First-In-First-Out) to record different types of information.
 *         This class only support two different kinds of syscall. One is PUSH, always
 *         insert an item into the back of list. Another is POP, always poping out the first
 *         item in the list.
 *         To avoid conflict, each thread should maintain all sorts of this information locally.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <dirent.h>
#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>

#include "threadstruct.hh"
#include "list.hh"
#include "log.hh"
#include "real.hh"
#include "recordentries.hh"
#include "xdefines.hh"

class SysRecord {
private:

  /**
   * In order to handle these record information uniformly, the first item is "list_t"
   */
  struct recordFile {
    list_t list;
    int fd;
		int ret;
  };

  struct recordMmap {
    void* addr;
  };

  struct recordMunmap {
    list_t list;
    void* addr;
    size_t length;
  };

  // sizeof(time_t) = 8
  struct recordTime {
    time_t time;
  };

  // sizeof(timeval) = 16, sizeof(timezone) = 8
  struct recordTimeofday {
    int ret;
    struct timeval tvalue;
    struct timezone tzone;
  };

  // sizeof(clock_t) = 8, sizeof(tms) =32
  struct recordTimes {
    clock_t ret;
    struct tms buf;
  };

  struct recordClone {
    int ret;
    pthread_t tid;
  };

  struct recordDir {
    list_t list;
    DIR* dir;
  };

public:

  // Record a file syscall according to given sc.
  void recordFileOps(SyscallType sc, int fd) {
		recordFileOps(sc, fd, 0);
	}

  void recordFileOps(SyscallType sc, int fd, int ret) {
    // using the assertion since only my code will call this.
    assert(sc <= E_SYS_FILE_DUP);

    struct recordFile* record = (struct recordFile*)allocEntry(sc);
    record->fd = fd;
    if(sc == E_SYS_FILE_CLOSE) {
			record->ret = ret;
      insertList(E_SYS_FILE_CLOSE, &record->list);
    }
  }

  // Get the fd with specific sc and remove corresponding item.
  bool getFileOps(SyscallType sc, int* fd) {
		int ret;

		// We don't care about the return value for this case
		return getFileOps(sc, fd, &ret);
	}

  bool getFileOps(SyscallType sc, int* fd, int * ret) {
    bool isFound = false;
    assert(sc <= E_SYS_FILE_DUP);
    struct recordFile* record = (struct recordFile*)retrieveEntry(sc);
    if(record) {
			if(sc == E_SYS_FILE_CLOSE) {
				*ret = record->ret;
			}
			else {
      	*fd = record->fd;
			}
      isFound = true;
    }
    return isFound;
  }

  bool retrieveFCloseList(int* fd) {
    bool isFound = false;
    struct recordFile* record = (struct recordFile*)retrieveListEntry(E_SYS_FILE_CLOSE);
    if(record) {
      *fd = record->fd;
      isFound = true;
    }
    return isFound;
  }

  // Record a file syscall according to given sc.
  void recordDirOps(SyscallType sc, DIR* dir) {
    // using the assertion since only my code will call this.
    assert(sc == E_SYS_DIR_OPEN || sc == E_SYS_DIR_CLOSE);

    struct recordDir* record = (struct recordDir*)allocEntry(sc);
    record->dir = dir;
    if(sc == E_SYS_DIR_CLOSE) {
      insertList(E_SYS_DIR_CLOSE, &record->list);
    }
  }

  // Get the fd with specific sc and remove corresponding item.
  bool getDirOps(SyscallType sc, DIR** dir) {
    bool isFound = false;
    assert(sc == E_SYS_DIR_OPEN || sc == E_SYS_DIR_CLOSE);
    struct recordDir* record = (struct recordDir*)retrieveEntry(sc);
    if(record) {
      *dir = record->dir;
      isFound = true;
    }
    return isFound;
  }

  bool retrieveDIRCloseList(DIR** dir) {
    bool isFound = false;
    struct recordDir* record = (struct recordDir*)retrieveListEntry(E_SYS_DIR_CLOSE);
    if(record) {
      *dir = record->dir;
      isFound = true;
    }
    return isFound;
  }

  // record time results
  void recordTimeOps(time_t time) {
    struct recordTime* record = (struct recordTime*)allocEntry(E_SYS_TIME);
    record->time = time;
  }

  // Get the first time results
  bool getTimeOps(time_t* time) {
    bool isFound = false;
    struct recordTime* record = (struct recordTime*)retrieveEntry(E_SYS_TIME);

    if(record) {
      *time = record->time;
      isFound = true;
    }

    return isFound;
  }

  // record time results
  void recordMmapOps(void* addr) {
    struct recordMmap* record = (struct recordMmap*)allocEntry(E_SYS_MMAP);
    record->addr = addr;
  }

  // Get the first time results
  bool getMmapOps(void** addr) {
    struct recordMmap* record = (struct recordMmap*)retrieveEntry(E_SYS_MMAP);
    bool isFound = false;

    if(record) {
      *addr = record->addr;
      isFound = true;
    }

    return isFound;
  }

  // record time results
  void recordMunmapOps(void* addr, size_t length) {
    struct recordMunmap* record = (struct recordMunmap*)allocEntry(E_SYS_MUNMAP);
    record->addr = addr;
    record->length = length;

    // Adding this munmap to the list.
    insertList(E_SYS_MUNMAP, &record->list);
  }

  // Get the first time results
  void getMunmapOps() {
    // We actually only update the list.
    /* struct recordMunmap* record = (struct recordMunmap*)*/
    retrieveEntry(E_SYS_MUNMAP); // FIXME unused return value.
    return;
  }

  // record time results
  void recordGettimeofdayOps(int ret, struct timeval* tv, struct timezone* tz) {
    struct recordTimeofday* record = (struct recordTimeofday*)allocEntry(E_SYS_GETTIMEOFDAY);

    // Save those tv and tz
    if(tv) {
      memcpy(&record->tvalue, tv, sizeof(struct timeval));
    }
    if(tz) {
      memcpy(&record->tzone, tz, sizeof(struct timezone));
    }
    record->ret = ret;
  }

  // Get the first time results
  bool getGettimeofdayOps(int* ret, struct timeval* tv, struct timezone* tz) {
    struct recordTimeofday* record = (struct recordTimeofday*)retrieveEntry(E_SYS_GETTIMEOFDAY);
    bool isFound = false;

    if(record) {
      // memcpy
      if(tv) {
        memcpy(tv, &record->tvalue, sizeof(struct timeval));
      }
      if(tz) {
        memcpy(tz, &record->tzone, sizeof(struct timezone));
      }
      *ret = record->ret;

      isFound = true;
    }

    return isFound;
  }

  // record time results
  void recordTimesOps(clock_t ret, struct tms* buf) {
    struct recordTimes* record = (struct recordTimes*)allocEntry(E_SYS_TIMES);

    // Save those tv and tz
    record->ret = ret;
    if(ret != -1) {
      memcpy(&record->buf, buf, sizeof(struct tms));
    }
  }

  // Get the first time results
  bool getTimesOps(clock_t* ret, struct tms* buf) {
    struct recordTimes* record = (struct recordTimes*)retrieveEntry(E_SYS_TIMES);
    bool isFound = false;

    if(record) {
      // memcpy
      *ret = record->ret;
      if(buf) {
        memcpy(buf, &record->buf, sizeof(struct tms));
      }

      isFound = true;
    }

    return isFound;
  }

  // record time results
  void recordCloneOps(int ret, pthread_t tid) {
    struct recordClone* rc = (struct recordClone*)allocEntry(E_SYS_CLONE);
    if(rc) {
      rc->ret = ret;
      rc->tid = tid;
    }
  }

  // Get the first time results
  bool getCloneOps(pthread_t* tid, int* ret) {
    bool isFound = false;
    struct recordClone* rc = (struct recordClone*)retrieveEntry(E_SYS_CLONE);

    if(rc) {
      *tid = rc->tid;
      *ret = rc->ret;
      isFound = true;
    }

    return isFound;
  }

  // For some list, we donot need to search one by one.
  // We can clear the whole list.
  static void epochBegin(DT::Thread *thread) {

		list_t * munmapList= &thread->syslist[E_SYS_MUNMAP];
		struct recordMunmap * record = NULL;

    while(true) {
			record = (struct recordMunmap*)listRetrieveItem(munmapList);

      if(record == NULL) {
        break;
      }

      Real::munmap(record->addr, record->length);
    }

		// Cleanup all record entries and all list of system calls;
		thread->syscalls.cleanup();
		for(int i = 0; i< E_SYS_MAX; i++) {
			listInit(&thread->syslist[i]);
		}
  }

  // Prepare the traverse for all list.
  static void prepareRollback(DT::Thread *thread) { thread->syscalls.prepareRollback(); }

private:

  inline void* allocEntry(SyscallType sc) {
    struct SyscallEntry* entry = current->syscalls.alloc();
    entry->syscall = sc;

		PRINF("SYSCALL: alloc entry %p for sc %d\n", (void *)entry, sc);
    return entry->data;
  }

  // For records, we only pop them from the beginning of a list
  void* retrieveEntry(SyscallType sc) {
    assert(sc >= E_SYS_FILE_OPEN && sc < E_SYS_MAX);

    struct SyscallEntry* entry = current->syscalls.retrieveIterEntry();
		
		PRINF("SYSCALL: retrieveEntry %p for sc %d\n", (void *)entry, sc);

		if(entry == NULL) {
      PRINF("Have we met this entry with sc %d\n", sc);
			assert(entry != NULL);
		}

    void* ptr = NULL;
    if(entry) {
      if(entry->syscall != sc) {
        PRINF("^^^^^^^^^^entry->syscall %d while we want system call %d^^^^^^^^^^^\n", entry->syscall, sc);
      	assert(entry->syscall == sc);
      }
      ptr = entry->data;
    }
    return ptr;
  }

  // Remove an entry from the list.
  void* retrieveListEntry(SyscallType sc) {
    assert(sc == E_SYS_FILE_CLOSE || sc == E_SYS_DIR_CLOSE || sc == E_SYS_MUNMAP);
    list_t* list = getTargetList(sc);
    return listRetrieveItem(list);
  }

  inline list_t* getTargetList(SyscallType sc) { return &current->syslist[sc]; }

  // We are always insert an entry into the tail of a list.
  void insertList(SyscallType sc, list_t* list) {
    list_t* head = getTargetList(sc);
    listInit(list);
    listInsertTail(list, head);
  }

private: 
	// We may have to keep two lists, currently.
	
};

#endif
