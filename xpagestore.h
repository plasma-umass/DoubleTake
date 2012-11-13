// -*- C++ -*-
#ifndef _XPAGESTORE_H_
#define _XPAGESTORE_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xplock.h"
#include "mm.h"
#include "xdefines.h"


class xpagestore {
public:
	xpagestore()
	: _start(NULL),
	  _cur(0)
	{ }

  // It is a perprocess instance since we are using the libraries' globals
  static xpagestore& getInstance (void) {
    static char buf[sizeof(xpagestore)];
    static xpagestore * theOneTrueObject = new (buf) xpagestore();
    return *theOneTrueObject;
  }
    
	void initialize(void) {
		_start = MM::mmapAllocatePrivate(xdefines::PageSize * xdefines::PRIVATE_PAGES, -1);
		if(_start == NULL)  {
			fprintf(stderr, "%d fail to initialize page store: %s\n", getpid(), strerror(errno));
			::abort();
		}

		_cur = 0;
		_total = xdefines::PRIVATE_PAGES;
	}

	void * alloc(void) {
		void * pageStart;

		if(_cur < _total) {
			pageStart = (void *)((intptr_t)_start + _cur * xdefines::PageSize);
			_cur++;
		}
 		else {
			// There is no enough entry now, re-allocate new entries now.
			fprintf(stderr, "%d: NO enough page, _cur %x _total %x!!!\n", getpid(), _cur, _total);
		}
		return pageStart;
  }

	void cleanup(void) {
		//fprintf(stderr, "%d : cleaning up _cur\n", getpid());
		_cur = 0;
	}

private:
	// Current index of entry that need to be allocated.
	int _cur;
	int _total;
	void *_start;
};

#endif
