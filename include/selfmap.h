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

#include <linux/limits.h>
#include <sys/resource.h>
#include <iostream>
#include <fstream>
#include <execinfo.h>
#include <stdint.h>

//#include <libunwind-ptrace.h>
//#include <sys/ptrace.h>

#define MAX_BUF_SIZE 4096
#include "xdefines.h"
using namespace std;

struct regioninfo {
  void * start;
  void * end; 
};

class selfmap {

public:
  static selfmap& getInstance() {
    static char buf[sizeof(selfmap)];
    static selfmap * theOneTrueObject = new (buf) selfmap();
    return *theOneTrueObject;
  }

  // We may use an inode attribute to analyze whether we need to do this.
  void getRegionInfo (std::string & curentry,
		      void ** start,
		      void ** end) 
  {

    // Now this entry is about globals of libc.so.
    string::size_type pos = 0;
    string::size_type endpos = 0;
    // Get the starting address and end address of this entry.
    // Normally the entry will be like this
    // "00797000-00798000 rw-p ...."
    string beginstr, endstr;

    while (curentry[pos] != '-') 
      pos++;

    beginstr = curentry.substr(0, pos);
  
    // Skip the '-' character
    pos++;
    endpos = pos;
    
    // Now pos should point to a space or '\t'.
    while(!isspace(curentry[pos])) pos++;
    endstr = curentry.substr(endpos, pos-endpos);

    // Save this entry to the passed regions.
    *start = (void *)strtoul(beginstr.c_str(), NULL, 16);
    *end = (void *)strtoul(endstr.c_str(), NULL, 16);

    return;
  }
  
  // @brief Get information about global regions. 
  void getTextRegions() {
    ifstream iMapfile;
    string curentry;

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRWRN("can't open /proc/self/maps, exit now\n");
      abort();
    } 

    // Now we analyze each line of the map file.
    void * startaddr, * endaddr;
    bool   hasLibStart = false;
    _appTextStart = (void *) (-1ULL);
    _appTextEnd = (void *) 0ULL;

    printf ("start = %lx, end = %x\n", _appTextStart, _appTextEnd);

    while (getline(iMapfile, curentry)) {
      size_t start, end, offset, inode;
      char file[PATH_MAX];
      char prot[15];
      unsigned int dev1, dev2;
      printf ("Read in %s\n", curentry.c_str());
      int count = sscanf (curentry.c_str(), "%lx-%lx %4s %lx %x:%x %u %s",
			  &start, &end, prot, &offset, &dev1, &dev2, &inode, file);
      if (count != 0) {
	printf ("start = %lx, end = %lx, prot = %s, offset = %lx, dev1 = %x, dev2 = %x, inode = %x, file = %s\n",
		start, end, prot, offset, dev1, dev2, inode, file);
      }

      // Check whether this entry is the text segment of application.
      if((curentry.find(" r-xp ", 0) != string::npos) 
	 && (curentry.find(" 08:0", 0) != string::npos)) {

	printf ("checking text segment.\n");
        getRegionInfo(curentry, &startaddr, &endaddr);

        // Check whether this is doubletake or application
        if (curentry.find("libdoubletake", 0) != string::npos) {
          _doubletakeStart = startaddr;
          _doubletakeEnd = endaddr;
          break;
        } else {
	  printf ("CHECKING.\n");
	  if ((size_t) startaddr < (size_t) _appTextStart) {
	    _appTextStart = startaddr;
	  }
	  if ((size_t) endaddr > (size_t) _appTextEnd) {
	    _appTextEnd = endaddr;
	  }
        } 
      } else if((curentry.find(" r-xp ") != string::npos) 
              && (curentry.find("lib", 0) != string::npos) 
              && (curentry.find(" 08:0") != string::npos)) {
        // Now it is start of global of applications
        getRegionInfo(curentry, &startaddr, &endaddr);

        if (!hasLibStart) {
          _libraryStart = startaddr;
          _libraryEnd = endaddr;
          hasLibStart = true;
        } else {
          _libraryEnd = endaddr;
        }
      }
    }
    iMapfile.close();

    printf ("NOW: start = %lx, end = %lx\n", _appTextStart, _appTextEnd);

    int    count;

    /* Get current executable file name. */
    count = Real::readlink()("/proc/self/exe", _filename, MAX_BUF_SIZE);
    if (count <= 0 || count >= MAX_BUF_SIZE)
    {
      PRWRN("Failed to get current executable file name\n" );
      exit(1);
    }
    _filename[count] = '\0';

    PRINF("INITIALIZATION: textStart %p textEnd %p doubletakeStart %p doubletakeEnd %p libStart %p libEnd %p\n", _appTextStart, _appTextEnd, _doubletakeStart, _doubletakeEnd, _libraryStart, _libraryEnd);
  } 

  // Check whether an address is inside the doubletake library itself
  bool isDoubleTakeLibrary(void * pcaddr) {
    if(pcaddr >= _doubletakeStart && pcaddr <= _doubletakeEnd) {
      return true;
    }
    else {
      return false;
    }
  }

  bool isApplication(void * pcaddr) {
    if(pcaddr >= _appTextStart && pcaddr <= _appTextEnd) {
      return true;
    }
    else {
      return false;
    }
  }

  /* Get the stack frame inside the tracer.
   * We can not simply use backtrace to get corresponding information
   * since it will give us the call stack of signal handler.
   * Not the watching processes that we need.
   * According to http://stackoverflow.com/questions/7258273/how-to-get-a-backtrace-like-gdb-using-only-ptrace-linux-x86-x86-64,
   * The following link can give us some clues to use libunwind.
    http://git.savannah.gnu.org/cgit/libunwind.git/plain/tests/test-ptrace.c?h=v1.0-stable
    http://git.savannah.gnu.org/cgit/libunwind.git/plain/tests/test-ptrace.c?h=v1.0-stable

   */
  // Print out the code information about an eipaddress
  // Also try to print out stack trace of given pcaddr.
  void printCallStack(ucontext_t * context, void * addr, bool isOverflow);
 
  // Trying to get information about global regions. 
  void getGlobalRegions(regioninfo * regions, int * regionNumb) {
    using namespace std;
    ifstream iMapfile;
    string curentry;

    //#define PAGE_ALIGN_DOWN(x) (((size_t) (x)) & ~xdefines::PAGE_SIZE_MASK)
    //void * globalstart = (void *)PAGE_ALIGN_DOWN(&__data_start);

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRWRN("can't open /proc/self/maps, exit now\n");
      abort();
    } 

    // Now we analyze each line of this maps file.
    bool toExit = false;
    bool toSaveRegion = false;
 
    void * startaddr, * endaddr;
    string nextentry;
    void * newstart;
    bool foundGlobals = false;

    while(getline(iMapfile, curentry)) {
      // Check the globals for the application. It is at the first entry
      if(foundGlobals == false && (curentry.find(" rw-p ") != string::npos)) {
        // Now it is start of global of applications
        getRegionInfo(curentry, &startaddr, &endaddr);
   
        getline(iMapfile, nextentry);

        void * newstart;
        // Check whether next entry should be also included or not.
        if(nextentry.find("lib", 0) == string::npos && nextentry.find(" 00:00 ") == string::npos) {
          getRegionInfo(nextentry, &newstart, &endaddr);
        }
        foundGlobals = true;
        toSaveRegion = true;
      }
      // Check the globals for libc.so
      else if((curentry.find("libc-", 0) != string::npos && (curentry.find(" rw-p ") != string::npos))) {
        getRegionInfo(curentry, &startaddr, &endaddr);
        getline(iMapfile, nextentry);
        getRegionInfo(nextentry, &newstart, &endaddr);
        toSaveRegion = true;
      }
#if 0
      // Check the globals for libpthread-***.so
      else if((curentry.find("libpthread-", 0) != string::npos && (curentry.find(" rw-p ") != string::npos))) {
        // The first entry can not intercepted since it includes the PLT table of mmap?
        getRegionInfo(curentry, &startaddr, &endaddr);
        getline(iMapfile, nextentry);
        getRegionInfo(nextentry, &newstart, &endaddr);
        toSaveRegion = true;
      }
#endif
      else {
        toSaveRegion = false;
      }
 
      if(toSaveRegion) { 
        PRINF("start startaddr %p endaddr %p\n", startaddr, endaddr); 
        regions[*regionNumb].start = startaddr;
        regions[*regionNumb].end = endaddr;
        (*regionNumb)++;
      }
    }
    iMapfile.close();
  } 
  
  // Trying to get stack information. 
  void getStackInformation(void** stackBottom, void ** stackTop) {
    using namespace std;
    ifstream iMapfile;
    string curentry;

    try {
      iMapfile.open("/proc/self/maps");
    } catch(...) {
      PRWRN("can't open /proc/self/maps, exit now\n");
      abort();
    } 

    // Now we analyze each line of this maps file.
    while(getline(iMapfile, curentry)) {

      // Find the entry for stack information.
      if(curentry.find("[stack]", 0) != string::npos) {
        string::size_type pos = 0;
        string::size_type endpos = 0;
        // Get the starting address and end address of this entry.
        // Normally the entry will be like this
        // ffce9000-ffcfe000 rw-p 7ffffffe9000 00:00 0   [stack]
        string beginstr, endstr;

        while(curentry[pos] != '-') pos++;
          beginstr = curentry.substr(0, pos);
  
          // Skip the '-' character
          pos++;
          endpos = pos;
        
          // Now pos should point to a space or '\t'.
          while(!isspace(curentry[pos])) pos++;
          endstr = curentry.substr(endpos, pos-endpos);

          // Now we get the start address of stack and end address
          *stackBottom = (void *)strtoul(beginstr.c_str(), NULL, 16);
          *stackTop = (void *)strtoul(endstr.c_str(), NULL, 16);
      }
    }
    iMapfile.close();
  }

private:
  char _filename[MAX_BUF_SIZE];
  void * _appTextStart;
  void * _appTextEnd;
  void * _libraryStart;
  void * _libraryEnd;
  void * _doubletakeStart;
  void * _doubletakeEnd; 
};

#endif
