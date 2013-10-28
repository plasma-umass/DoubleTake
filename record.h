// -*- C++ -*-
/*
  Copyright (c) 2012, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   recordsyscalls.h
 * @brief  A simple list (First-In-First-Out) to record different types of information. 
 *         This class only support two different kinds of operation. One is PUSH, always
 *         insert an item into the back of list. Another is POP, always poping out the first
 *         item in the list.
 *         To avoid conflict, each thread should maintain all sorts of this information locally.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _RECORD_H_
#define _RECORD_H_

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>

#include "xdefines.h"
#include "recordentries.h"
#include "internalheap.h"

class Record {
public:
  typedef enum e_recordOps {
    E_OP_FILE_OPEN = 0,
    E_OP_FILE_CLOSE,
    E_OP_FILE_DUP,
    E_OP_DIR_OPEN,
    E_OP_DIR_CLOSE,
    E_OP_MMAP,
    E_OP_MUNMAP,
    E_OP_TIME,
    E_OP_GETTIMEOFDAY,
    E_OP_TIMES,
    E_OP_CLONE,
    E_OP_MAX
  } eRecordOps;

private:
  class RecordEntry {
  public:
    eRecordOps operation; 
    char  data[64 - sizeof(eRecordOps)];
  };

  /** 
   * In order to handle these record information uniformly, the first item is "list_t"
   */
  struct recordFile {
    list_t list;
    int fd;
  };

  struct recordMmap {
    void * addr;
  };
  
  struct recordMunmap {
    list_t list;
    void * addr;
    size_t length;
  };

  // sizeof(time_t) = 8
  struct recordTime {
    time_t time;
  };

  // sizeof(timeval) = 16, sizeof(timezone) = 8
  struct recordTimeofday {
    int    ret;
    struct timeval tvalue;
    struct timezone tzone;
  };

  // sizeof(clock_t) = 8, sizeof(tms) =32
  struct recordTimes {
    clock_t ret;
    struct tms buf; 
  };

  struct recordClone {
    int    ret;
    pthread_t tid;
  };

  struct recordDir {
    list_t list;
    DIR * dir;
  };

public:

  void initialize(void) {
//    fprintf(stderr, "RECORD: _entries %p\n", &_entries);
    _entries.initialize();

    // Initialize corresponding lists
    for(int i = 0; i < E_OP_MAX; i++) {
      listInit(&_glist[i]);
    }
    _lck.init();
  }

  // Record a file operation according to given op.
  void recordFileOps(eRecordOps op, int fd) {
    // using the assertion since only my code will call this.
    assert(op <= E_OP_FILE_DUP);

    struct recordFile * record = (struct recordFile *)allocEntry(op);
    record->fd = fd;
    if(op == E_OP_FILE_CLOSE) {
      insertList(E_OP_FILE_CLOSE, &record->list);
    }
  }

  // Get the fd with specific op and remove corresponding item.
  bool getFileOps(eRecordOps op, int * fd) {
    bool isFound = false;
    assert(op <= E_OP_FILE_DUP);
    struct recordFile * record = (struct recordFile *)getEntry(op);
    if(record) {
      *fd = record->fd;
      isFound = true;
    }
    return isFound;
  }
 
  bool retrieveFCloseList(int * fd) {
    bool isFound = false;
    struct recordFile * record = (struct recordFile *)retrieveListEntry(E_OP_FILE_CLOSE);
    if(record) {
      *fd = record->fd;
      isFound = true;
    }
    return isFound;
  }
 
  // Record a file operation according to given op.
  void recordDirOps(eRecordOps op, DIR * dir) {
    // using the assertion since only my code will call this.
    assert(op == E_OP_DIR_OPEN || op == E_OP_DIR_CLOSE);

    struct recordDir * record = (struct recordDir *)allocEntry(op);
    record->dir = dir;
    if(op == E_OP_DIR_CLOSE) {
      insertList(E_OP_DIR_CLOSE, &record->list);
    }
  }

  // Get the fd with specific op and remove corresponding item.
  bool getDirOps(eRecordOps op, DIR ** dir) {
    bool isFound = false;
    assert(op == E_OP_DIR_OPEN || op == E_OP_DIR_CLOSE);
    struct recordDir * record = (struct recordDir *)getEntry(op);
    if(record) {
      *dir = record->dir;
      isFound = true;
    }
    return isFound;
  }
  
  bool retrieveDIRCloseList(DIR ** dir) {
    bool isFound = false;
    struct recordDir * record = (struct recordDir *)retrieveListEntry(E_OP_DIR_CLOSE);
    if(record) {
      *dir = record->dir;
      isFound = true;
    }
    return isFound;
  }
 

  // record time results
  void recordTimeOps(time_t time) {
    struct recordTime * record = (struct recordTime *)allocEntry(E_OP_TIME);
    record->time = time;
  }

  // Get the first time results
  bool getTimeOps(time_t * time) {
    bool isFound = false;
    struct recordTime * record = (struct recordTime *)getEntry(E_OP_TIME);

    if(record) {
      *time = record->time;
      isFound = true;
    }

    return isFound;
  }
    
  // record time results
  void recordMmapOps(void * addr) {
    struct recordMmap * record = (struct recordMmap *)allocEntry(E_OP_MMAP);
    record->addr = addr;
  }

  // Get the first time results
  bool getMmapOps(void ** addr) {
    struct recordMmap * record = (struct recordMmap *)getEntry(E_OP_MMAP);
    bool isFound = false;
    
    if(record) {
      *addr = record->addr;
      isFound = true;
    }

    return isFound;
  }
 
  // record time results
  void recordMunmapOps(void * addr, size_t length) {
    struct recordMunmap * record = (struct recordMunmap *)allocEntry(E_OP_MUNMAP);
    record->addr = addr;
    record->length = length;

    // Adding this munmap to the list.
    insertList(E_OP_MUNMAP, &record->list);
  }

  // Get the first time results
  bool getMunmapOps(void ** addr, size_t * length) {
    bool isFound = false;
    struct recordMunmap * record = (struct recordMunmap *)getEntry(E_OP_MUNMAP);
    
    if(record) {
      *addr = record->addr;
      *length = record->length;

      isFound = true;
    }
    
    return isFound;
  }

  // record time results
  void recordGettimeofdayOps(int ret, struct timeval *tv, struct timezone *tz) { 
    eRecordOps op = E_OP_GETTIMEOFDAY; 
    struct recordTimeofday * record = (struct recordTimeofday *)allocEntry(E_OP_GETTIMEOFDAY);
    
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
  bool getGettimeofdayOps(int * ret, struct timeval * tv, struct timezone * tz) {
    struct recordTimeofday * record = (struct recordTimeofday *)getEntry(E_OP_GETTIMEOFDAY);
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
  void recordTimesOps(clock_t ret, struct tms * buf) { 
    struct recordTimes * record = (struct recordTimes *)allocEntry(E_OP_TIMES);
    
    // Save those tv and tz
    record->ret = ret;
    if(ret != -1) {
      memcpy(&record->buf, buf, sizeof(struct tms));
    }
  }

  // Get the first time results
  bool getTimesOps(clock_t * ret, struct tms * buf) {
    struct recordTimes * record = (struct recordTimes *)getEntry(E_OP_TIMES);
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
    struct recordClone * rc = (struct recordClone *)allocEntry(E_OP_CLONE);
    if(rc) {
      rc->ret = ret;
      rc->tid = tid;
    }
  }

  // Get the first time results
  bool getCloneOps(pthread_t * tid, int * ret) {
    bool isFound = false;
    struct recordClone * rc = (struct recordClone *)getEntry(E_OP_CLONE);
   
    if(rc) {
      *tid = rc->tid;
      *ret = rc->ret;
      isFound = true;
    }

    return isFound;
  }
   
  // For some list, we donot need to search one by one.
  // We can clear the whole list.
  void epochBegin(void) {
    // Do munmap for all entries
    struct recordMunmap * record = NULL;
    while(true) {
      record = (struct recordMunmap *)retrieveListEntry(E_OP_MUNMAP);

      if(record == NULL) {
        break;
      }
    
      WRAP(munmap)(record->addr, record->length);
    }
    _entries.cleanup();
  }

  // Prepare the traverse for all list.
  void prepareRollback(void) {
    _entries.prepareRollback();
  }

private:

  void lock(void) {
    _lck.lock();
  }

  void unlock(void) {
    _lck.unlock();
  }

  inline void * allocEntry(eRecordOps op) {
    lock();
    class RecordEntry * entry = _entries.alloc();
    unlock();
    entry->operation = op;
    return entry->data; 
  }
  
  // For records, we only pop them from the beginning of a list
  void * getEntry (eRecordOps op) {
    assert(op >=E_OP_FILE_OPEN && op < E_OP_MAX);

    class RecordEntry * entry = _entries.retrieveIterEntry();

    void * ptr = NULL; 
    if(entry) {
      assert(entry->operation == op);
      ptr = entry->data;
    }
    return ptr;    
  }

  // Remove an entry from the list.
  void * retrieveListEntry(eRecordOps op) {
    assert(op == E_OP_FILE_CLOSE || op == E_OP_DIR_CLOSE || op == E_OP_MUNMAP);
    list_t * list = getTargetList(op);
    return listRetrieveItem(list);
  }

  inline list_t * getTargetList(e_recordOps op) {
    return &_glist[op];
  }

  // We are always insert an entry into the tail of a list.
  void insertList(e_recordOps op, list_t * list) {
    list_t * head = getTargetList(op);
    listInit(list);
    listInsertTail(list, head);
  }

  RecordEntries<class RecordEntry, xdefines::MAX_RECORD_ENTRIES>  _entries;
  list_t _glist[E_OP_MAX];
  spinlock _lck;
};

#endif


