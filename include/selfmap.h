// -*- C++ -*-

/*
  Copyright (c) 2013, University of Massachusetts Amherst.

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
 * @file   selfmap.h
 * @brief  Process the /proc/self/map file.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

#ifndef DOUBLETAKE_SELFMAP_H
#define DOUBLETAKE_SELFMAP_H

#include <assert.h>
#include <linux/limits.h>
#include <sys/resource.h>
#include <iostream>
#include <fstream>
#include <execinfo.h>
#include <stdint.h>

//#include <libunwind-ptrace.h>
//#include <sys/ptrace.h>

#include "xdefines.h"
using namespace std;

struct regioninfo {
  void * start;
  void * end; 
};

class selfmap {
public:

  /// @brief Check whether an address is inside the DoubleTake library itself.
  bool isDoubleTakeLibrary (void * pcaddr) {
    return ((pcaddr >= _doubletakeStart) && (pcaddr <= _doubletakeEnd));
  }

  /// @brief Check whether an address is inside the main application.
  bool isApplication(void * pcaddr) {
    return ((pcaddr >= _appTextStart) && (pcaddr <= _appTextEnd));
  }

private:

  // A class that represents process information, extracted from the maps file.
  class pmap {
  public:

    /// @brief Extract information from one line of a /proc/<pid>/maps file.
    pmap (const char * str)
    {
      sscanf (str, "%lx-%lx %4s %lx %x:%x %u %s",
	      &startaddr, &endaddr, prot, &offset, &dev1, &dev2, &inode, file);
    }
  
    size_t startaddr, endaddr, offset, inode;
    char file[PATH_MAX];
    char prot[15];
    unsigned int dev1, dev2;
  };

public:

  static selfmap& getInstance() {
    static char buf[sizeof(selfmap)];
    static selfmap * theOneTrueObject = new (buf) selfmap();
    return *theOneTrueObject;
  }

  // Print out the code information about an eip address.
  // Also try to print out the stack trace of given pcaddr.
  void printCallStack (ucontext_t * context, void * addr, bool isOverflow);
 
  /// @brief Get information about global regions. 
  void getTextRegions() 
  {
    ifstream iMapfile;
    string curentry;

    // Get application file name.
    int count = Real::readlink()("/proc/self/exe", _filename, PATH_MAX);
    if (count <= 0 || count >= PATH_MAX)
      {
	PRWRN("Failed to get current executable file name\n" );
	exit(1);
      }
    _filename[count] = '\0';

    // Read the maps to compute ranges for text regions (code).
    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRWRN("can't open /proc/self/maps, exit now\n");
      abort();
    } 

    // Now we analyze each line of the map file.
    _appTextStart = (void *) (-1ULL);  // largest possible address
    _appTextEnd   = (void *) 0ULL;     // smallest possible address
    _libraryStart    = _appTextStart;
    _libraryEnd      = _appTextEnd;

    printf ("start = %lx, end = %x\n", _appTextStart, _appTextEnd);

    while (getline(iMapfile, curentry)) {

      pmap p (curentry.c_str());

      // Check whether this entry is the text segment of application.
      if (strcmp (p.prot, "r-xp") == 0) {
	// We're in.
	printf ("checking text segment.\n");

        // Check whether we are in DoubleTake, another library, or in the application itself.
        if (strstr(p.file, "libdoubletake") != NULL) {
          _doubletakeStart = (void *) p.startaddr;
          _doubletakeEnd   = (void *) p.endaddr;
        } else if (strcmp(p.file, _filename) == 0) {
	  if (p.startaddr < (size_t) _appTextStart) {
	    _appTextStart = (void *) p.startaddr;
	  }
	  if (p.endaddr > (size_t) _appTextEnd) {
	    _appTextEnd = (void *) p.endaddr;
	  }
        } else {
	  // Must be in a library.
	  if (p.startaddr < (size_t) _libraryStart) {
	    _libraryStart = (void *) p.startaddr;
	  }
	  if (p.endaddr > (size_t) _libraryEnd) {
	    _libraryEnd = (void *) p.endaddr;
	  }
	}
      }

      iMapfile.close();

      PRINF("INITIALIZATION: textStart %p textEnd %p doubletakeStart %p doubletakeEnd %p libStart %p libEnd %p\n", _appTextStart, _appTextEnd, _doubletakeStart, _doubletakeEnd, _libraryStart, _libraryEnd);
    } 
  }

  /// @brief Extract stack bottom and top information.
  void getStackInformation(void** stackBottom, void ** stackTop) {
    using namespace std;
    ifstream iMapfile;
    string curentry;

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRWRN("can't open /proc/self/maps, exiting now.\n");
      abort();
    } 

    *stackBottom = NULL;
    *stackTop = NULL;

    // Now we analyze each line of this maps file, looking for the stack.
    while (getline(iMapfile, curentry)) {

      pmap p (curentry.c_str());

      // Find the entry for the stack.
      if (strstr(p.file, "[stack]") != NULL) {
	*stackBottom = (void *) p.startaddr;
	*stackTop    = (void *) p.endaddr;
	// Got it. We're out.
	break;
      }
    }
    iMapfile.close();
    assert (*stackBottom != NULL);
    assert (*stackTop != NULL);
  }

  /// @brief Collect all global regions.
  void getGlobalRegions(regioninfo * regions, int * regionNumb) {
    using namespace std;
    ifstream iMapfile;
    string curentry;

    //#define PAGE_ALIGN_DOWN(x) (((size_t) (x)) & ~xdefines::PAGE_SIZE_MASK)
    //void * globalstart = (void *)PAGE_ALIGN_DOWN(&__data_start);

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRWRN("can't open /proc/self/maps, exiting.\n");
      abort();
    } 

    // Now we analyze each line of this maps file.
    bool toExit = false;
    bool toSaveRegion = false;
 
    string nextentry;
    void * newstart;
    bool foundGlobals = false;

    while (getline(iMapfile, curentry)) {

      pmap p (curentry.c_str());

      // Check the globals for the application. It is the first entry.
      if ((foundGlobals == false) && (strstr(p.prot, "rw-p") != NULL)) {
        // Now it is start of global of applications.
   
	if(toSaveRegion) { 
	  PRINF("start startaddr %p endaddr %p\n", p.startaddr, p.endaddr); 
	  regions[*regionNumb].start = (void *) p.startaddr;
	  regions[*regionNumb].end   = (void *) p.endaddr;
	  (*regionNumb)++;
	}
      }
    }
    iMapfile.close();
  }
  
private:
  
  char _filename[PATH_MAX];
  void * _appTextStart;
  void * _appTextEnd;
  void * _libraryStart;
  void * _libraryEnd; 
  void * _doubletakeStart;
  void * _doubletakeEnd; 
};

#endif
