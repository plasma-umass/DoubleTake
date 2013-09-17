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
#include "internalheap.h"

class Record {

  struct recordHeader {
    list_t list;
    list_t * curentry;  // Used to traverse the list in rollback.
  };

  /** 
   * In order to handle these record information uniformly, the first item is "list_t"
   */
  struct recordFile {
    list_t list;
    int fd;
  };

  struct recordMmap {
    list_t list;
    void * addr;
  };
  
  struct recordMunmap {
    list_t list;
    void * addr;
    size_t length;
  };


  struct recordTime {
    list_t list;
    time_t time;
  };

  struct recordTimeofday {
    list_t list;
    int    ret;
    struct timeval tvalue;
    struct timezone tzone;
  };

  struct recordTimes {
    list_t list;
    clock_t ret;
    struct tms buf; 
  };

  struct recordClone {
    list_t list;
    int    ret;
    pthread_t tid;
  };

  struct recordDir {
    list_t list;
    DIR * dir;
  };

public:

  typedef enum recordOps {
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
  } e_recordOps;

  void initialize(void) {
    struct recordHeader * record;

    for(int i = 0; i < E_OP_MAX; i++) {
      record = &_glist[i];
      listInit(&record->list);
    } 
  }

  // Record a file operation according to given fops.
  void recordFileOps(e_recordOps fops, int fd) {
    // using the assertion since only my code will call this.
    assert(fops <= E_OP_FILE_DUP);

    struct recordFile * rf = (struct recordFile *)malloc(sizeof(*rf));
    rf->fd = fd;
    listInit(&rf->list);

    // Insert to the list
    insertList(fops, (list_t *)&rf->list); 
  }

  // Get the fd with specific fops and remove corresponding item.
  bool getFileOps(e_recordOps fops, int * fd) {
    bool isFound = false;
    assert(fops <= E_OP_FILE_DUP);
    struct recordFile * rf = (struct recordFile *)getNextEntry(fops);
    if(rf) {
      *fd = rf->fd;
      isFound = true;
    }
    return isFound;
  }
  
  // Record a file operation according to given fops.
  void recordDirOps(e_recordOps dops, DIR * dir) {
    // using the assertion since only my code will call this.
    assert(dops == E_OP_DIR_OPEN || dops == E_OP_DIR_CLOSE);

    struct recordDir * rd = (struct recordDir *)malloc(sizeof(*rd));
    rd->dir = dir;
    listInit(&rd->list);

    // Insert to the list
    insertList(dops, (list_t *)&rd->list); 
  }

  // Get the fd with specific fops and remove corresponding item.
  bool getDirOps(e_recordOps dops, DIR ** dir) {
    bool isFound = false;
    assert(dops == E_OP_DIR_OPEN || dops == E_OP_DIR_CLOSE);
    struct recordDir * rd = (struct recordDir *)getNextEntry(dops);
    if(rd) {
      *dir = rd->dir;
      isFound = true;
    }
    return isFound;
  }

  // record time results
  void recordTimeOps(time_t time) {
    struct recordTime * rt = (struct recordTime *)malloc(sizeof(*rt));
    rt->time = time;
    listInit(&rt->list);

    // Insert to the list
    insertList(E_OP_TIME, (list_t *)&rt->list); 
  }

  // Get the first time results
  bool getTimeOps(time_t * time) {
    bool isFound = false;
    e_recordOps op = E_OP_TIME;
    struct recordTime * rt = (struct recordTime *)getNextEntry(op);

    if(rt) {
      *time = rt->time;
      isFound = true;
    }

    return isFound;
  }
    
  // record time results
  void recordMmapOps(void * addr) {
    struct recordMmap * rm = (struct recordMmap *)malloc(sizeof(*rm));
    rm->addr = addr;
    listInit(&rm->list);

    // Insert to the list
    insertList(E_OP_MMAP, (list_t *)&rm->list); 
  }

  // Get the first time results
  bool getMmapOps(void ** addr) {
    struct recordMmap * rm = (struct recordMmap *)getNextEntry(E_OP_MMAP);
    bool isFound = false;
    
    if(rm) {
      *addr = rm->addr;
      isFound = true;
    }

    return isFound;
  }
 
  // record time results
  void recordMunmapOps(void * addr, size_t length) {
    struct recordMunmap * rm = (struct recordMunmap *)malloc(sizeof(*rm));
    rm->addr = addr;
    rm->length = length;
    listInit(&rm->list);

    // Insert to the list
    insertList(E_OP_MUNMAP, (list_t *)&rm->list); 
  }

  // Get the first time results
  bool getMunmapOps(void ** addr, size_t * length) {
    bool isFound = false;
    struct recordMunmap * rm = (struct recordMunmap *)getNextEntry(E_OP_MUNMAP);
    
    if(rm) {
      *addr = rm->addr;
      *length = rm->length;

      isFound = true;
    }
    
    return isFound;
  }

  // record time results
  void recordGettimeofdayOps(int ret, struct timeval *tv, struct timezone *tz) { 
    e_recordOps op = E_OP_GETTIMEOFDAY; 
    struct recordTimeofday * rt = (struct recordTimeofday *)malloc(sizeof(*rt));
    
    // Save those tv and tz
    if(tv) {
      memcpy(&rt->tvalue, tv, sizeof(struct timeval));
    }
    if(tz) {
      memcpy(&rt->tzone, tz, sizeof(struct timezone));
    }
    rt->ret = ret;
    listInit(&rt->list);

    // Insert to the list
    insertList(op, (list_t *)&rt->list); 
  }

  // Get the first time results
  bool getGettimeofdayOps(int * ret, struct timeval * tv, struct timezone * tz) {
    e_recordOps op = E_OP_GETTIMEOFDAY; 
    struct recordTimeofday * rt = (struct recordTimeofday *)getNextEntry(op);
    bool isFound = false;

    if(rt) {
      // memcpy
      if(tv) { 
        memcpy(tv, &rt->tvalue, sizeof(struct timeval));
      }
      if(tz) {
        memcpy(tz, &rt->tzone, sizeof(struct timezone));
      }
      *ret = rt->ret;

      isFound = true;
    }

    return isFound;
  }
  
  // record time results
  void recordTimesOps(clock_t ret, struct tms * buf) { 
    e_recordOps op = E_OP_TIMES; 
    struct recordTimes * rt = (struct recordTimes *)malloc(sizeof(*rt));
    
    // Save those tv and tz
    rt->ret = ret;
    if(ret != -1) {
      memcpy(&rt->buf, buf, sizeof(struct tms));
    }
    listInit(&rt->list);

    // Insert to the list
    insertList(op, (list_t *)&rt->list); 
  }

  // Get the first time results
  bool getTimesOps(clock_t * ret, struct tms * buf) {
    e_recordOps op = E_OP_TIMES; 
    struct recordTimes * rt = (struct recordTimes *)getNextEntry(op);
    bool isFound = false;

    if(rt) {
      // memcpy
      *ret = rt->ret;
      if(buf) { 
        memcpy(buf, &rt->buf, sizeof(struct tms));
      }

      isFound = true;
    }

    return isFound;
  }

#if 1    
  // record time results
  void recordCloneOps(int ret, pthread_t tid) {
    struct recordClone * rc = (struct recordClone *)malloc(sizeof(*rc));
    if(rc) {
      rc->ret = ret;
      rc->tid = tid;
      listInit(&rc->list);
    }

    // Insert to the list
    insertList(E_OP_CLONE, (list_t *)&rc->list); 
  }

  // Get the first time results
  bool getCloneOps(pthread_t * tid, int * ret) {
    bool isFound = false;
    struct recordClone * rc = (struct recordClone *)getNextEntry(E_OP_CLONE);
   
    if(rc) {
      *tid = rc->tid;
      *ret = rc->ret;
      isFound = true;
    }

    return isFound;
  }
#endif 
   
  // For some list, we donot need to search one by one.
  // We can clear the whole list.
  void clearRecordList(e_recordOps op) {
    assert(op >= E_OP_FILE_OPEN && op <= E_OP_MAX);

    list_t * head = getTargetList(op);
    list_t * entry = NULL;

    while((entry = listRetrieveItem(head)) != NULL) {
      freeEntry(entry); 
    }

    listInit(head);
  }

  // Prepare the traverse for all list.
  void prepareTraverse(void) {
    int i;
    struct recordHeader * record;

    for(i = 0; i < E_OP_MAX; i++) {
      record = &_glist[i];    

      if(!isListEmpty(&record->list)) {
        record->curentry = nextEntry(&record->list);
      }
      else {
        record->curentry = NULL;
      }
    }
  }

private:

  inline void * malloc(size_t sz) {
    return InternalHeap::getInstance().malloc(sz);
  }
  
  inline void free(void * ptr) {
    InternalHeap::getInstance().free(ptr);
  }
 
  // We are always insert an entry into the tail of a list.
  void insertList(e_recordOps op, list_t * list) {
    list_t * head = getTargetList(op);
    
    listInsertTail(list, head);
  }

  // For records, we only pop them from the beginning of a list
  list_t * getNextEntry (e_recordOps op) {
    assert(op >=E_OP_FILE_OPEN && op < E_OP_MAX);
    
    struct recordHeader * record = &_glist[op];

    //PRWRN("debug: op is %d record %p\n", op, record);
    list_t * curentry = record->curentry;
      
    if(curentry && !isListTail(curentry, &record->list)) {
      record->curentry = nextEntry(curentry);
    }
    else {
      record->curentry = NULL;
    }
    return curentry;    
  }

  void freeEntry(void * ptr) {
    free(ptr);
  }
  
  inline list_t * getTargetList(e_recordOps op) {
    return &_glist[op].list;
  }

  struct recordHeader _glist[E_OP_MAX];  
};

#endif


