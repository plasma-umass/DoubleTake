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
 * @file   semaphore.h
 * @brief  Semaphore used to reproduce the order of synchronization.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "xdefines.h"
#include "real.h"

extern "C" {
  typedef union semun {
    int val;
    struct semid_ds *buf;
    ushort * array;
  } semaArg;
};
  
class semaphore {
public:
  semaphore() {
  }

  ~semaphore() {
    destroy();
  }

#ifdef X86_32BIT
  int createSemaKey(unsigned long key) {
    return key;
  }
#else
  int createSemaKey(unsigned long key) {
    int key1 = (int)key;
    int key2 = (int)((key >> 16) & 0xFFFFFFFF);

    return key1 + key2;
  }
#endif 
 
  void init(unsigned long key, int nsemas, int initValue) 
  {
    int id;

    // We are trying to check whether a key is existing. If yes, then we 
    // must pickup another key.
    _semaKey = createSemaKey(key);  
    while(true) {
      /* Create the semaphore with external key KEY if it doesn't already 
       exists. Give permissions to the world. */
      id = Real::semget()(_semaKey, 1, 0666);
      /* Always check system returns. */
      if(id < 0)
      {
        break; 
      }
      _semaKey++; 
    } 

    // Now we are guaranteed that the key is not exisiting.
    semaArg arg;
 
    // Initialize semaphore to the desired number.
    arg.val = initValue;
    _semaId = Real::semget()(_semaKey, nsemas, 0666 | IPC_CREAT);
    //DEBUG("Semaphore %p _semaId %d semaphore creation\n", &_semaKey, _semaId);
    if(_semaId == -1) {
      DEBUG("semaphore creates failure %s\n", strerror(errno));
      abort();
    }
    else {
      Real::semctl()(_semaId, 0, SETVAL, arg);
    }
  }
  
  void wait(int val) {
    int minusVal = 0 - val;
    set(minusVal);
  }
  
  // Put and get are only used for simple increment and decrement operation.
  void get() {
    set(-1);
  }
  
  void put() {
    set(1);
  }
  
  void destroy()
  {
    semaArg argument;
    argument.val = 0;
 
    if(_semaId == 0) {
      return;
    }
 
    //DEBUG("Semaphore %p _semaId %d semaphore destory\n", &_semaKey, _semaId);
    if(Real::semctl()(_semaId, 0, IPC_RMID, argument) < 0)
    {
      DEBUG("Cannot detroy semaphore.\n");
      abort();  
    }
  }

private:
  void set (int val) {
    //DEBUG("Semaphore %p _semaId %d semop value %d\n", &_semaKey, _semaId, val);
    // Now increment the semaphore by the desired value.
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = val;
    sops.sem_flg = 0;
    int retval = Real::semop()(_semaId, &sops, 1);
  }
  
  int _semaKey;
  int _semaId;
};
#endif
