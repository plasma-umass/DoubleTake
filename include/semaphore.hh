#if !defined(DOUBLETAKE_SEMAPHORE_H)
#define DOUBLETAKE_SEMAPHORE_H

/*
 * @file   semaphore.h
 * @brief  Semaphore used to reproduce the order of synchronization.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>

#include "log.hh"
#include "real.hh"

typedef union semun {
  int val;
  struct semid_ds* buf;
  ushort* array;
} semaArg;

class semaphore {
public:
  semaphore() {}

  ~semaphore() { destroy(); }

#ifdef X86_32BIT
  int createSemaKey(unsigned long key) { return key; }
#else
  int createSemaKey(unsigned long key) {
    int key1 = (int)key;
    int key2 = (int)((key >> 16) & 0xFFFFFFFF);

    return key1 + key2;
  }
#endif

  void init(unsigned long key, int nsemas, int initValue) {
    int id;

    // We are trying to check whether a key is existing. If yes, then we
    // must pickup another key.
    _semaKey = createSemaKey(key);
    while(true) {
      /* Create the semaphore with external key KEY if it doesn't already
	 exist. Give permissions to the world. */
      id = Real::semget(_semaKey, 1, 0666);
      /* Always check system returns. */
      if(id < 0) {
        break;
      }
      _semaKey++;
    }

    // Now we are guaranteed that the key is not exisiting.
    semaArg arg;

    // Initialize semaphore to the desired number.
    arg.val = initValue;
    _semaId = Real::semget(_semaKey, nsemas, 0666 | IPC_CREAT);
    if(_semaId == -1) {
      PRWRN("semaphore creation failure %s\n", strerror(errno));
      // EDB FIXME: for now, do nothing.
      // abort();
    } else {
      Real::semctl(_semaId, 0, SETVAL, arg);
    }
  }

  void wait(int val) {
    int minusVal = 0 - val;
    set(minusVal);
  }

  // Put and get are only used for simple increment and decrement operation.
  void get() { set(-1); }

  void put() {
		//PRINT("someone up the semaphore on semid %d\n", _semaId);  
		set(1); 
	}

  void destroy() {
    semaArg argument;
    argument.val = 0;

    if(_semaId == 0) {
      return;
    }

    // PRWRN("Semaphore %p _semaId %d semaphore destory\n", &_semaKey, _semaId);
    if(Real::semctl(_semaId, 0, IPC_RMID, argument) < 0) {
      PRWRN("Cannot detroy semaphore.\n");
      abort();
    }
  }

private:
  void set(int val) {
    // PRINF("Semaphore %p _semaId %d semop value %d\n", &_semaKey, _semaId, val);
    // Now increment the semaphore by the desired value.
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = val;
    sops.sem_flg = 0;
    /* int retval = */ Real::semop(_semaId, &sops, 1); // FIXME? throwing away return value.
  }

  int _semaKey;
  int _semaId;
};

#endif
