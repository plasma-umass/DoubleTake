#if !defined(DOUBLETAKE_SELFMAP_H)
#define DOUBLETAKE_SELFMAP_H

/*
 * @file   selfmap.h
 * @brief  Process the /proc/self/map file.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <functional>
#include <map>
#include <new>
#include <string>
#include <utility>

#include "doubletake.hh"

#include "interval.hh"
#include "mm.hh"
#include "real.hh"

// From heaplayers
#include "wrappers/stlallocator.h"

using doubletake::RegionInfo;
using namespace std;

/**
 * A single mapping parsed from the /proc/self/maps file
 */
class mapping {
public:
  mapping()
    : _file(""), _base(0), _limit(0), _offset(0), _valid(false),
      _readable(false), _writable(false), _copyOnWrite(false) {}

  mapping(uintptr_t base,
	  uintptr_t limit,
	  char* perms,
	  size_t offset,
	  std::string file)
    : _file(file),
      _base(base),
      _limit(limit),
      _offset(offset),
      _valid(true),
      _readable(perms[0] == 'r'),
      _writable(perms[1] == 'w'),
      _executable(perms[2] == 'x'),
      _copyOnWrite(perms[3] == 'p'),
      _isStack(file.compare(0, strlen("[stack"), "[stack") == 0),
      // global mappings are RW_P, and either the heap, or the mapping is backed
      // by a file (and all files have absolute paths)
      _isGlobals((_readable && _writable && !_executable && _copyOnWrite) &&
                 (_file.size() > 0 && (_file == "[heap]" || _file[0] == '/'))) {}

  bool valid() const { return _valid; }

  bool isText() const { return _readable && !_writable && _executable; }
  bool isStack() const { return _isStack; }
  bool isGlobals(std::string mainfile) const { return _isGlobals; }

  size_t getOffset() const { return _offset; }
  uintptr_t getBase() const { return _base; }
  uintptr_t getLimit() const { return _limit; }

  const std::string& getFile() const { return _file; }

private:
  std::string _file;
  uintptr_t _base;
  uintptr_t _limit;
  size_t _offset;
  bool _valid;
  bool _readable;
  bool _writable;
  bool _executable;
  bool _copyOnWrite;
  bool _isStack;
  bool _isGlobals;
};

class selfmap {
public:
  selfmap();

  /// Check whether an address is inside the DoubleTake library itself.
  bool isDoubleTake(void* pcaddr) const {
    return ((pcaddr >= _doubletakeStart) && (pcaddr <= _doubletakeEnd));
  }

  /// Check whether an address is inside the main application.
  bool isApplication(void* pcaddr) const {
    return ((pcaddr >= _appTextStart) && (pcaddr <= _appTextEnd));
  }

  // Print out the code information about an eip address.
  // Also try to print out the stack trace of given pcaddr.
  void printStackCurrent();
  void printStack(int depth, void** array);

  int findStack(pid_t tid, uintptr_t *bottom, uintptr_t *top) {
    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;
      // TODO: also test tid
      if(m.isStack()) {
        *bottom = m.getBase();
        *top = m.getLimit();
        return 0;
      }
    }
    fprintf(stderr, "Couldn't find stack mapping. Giving up.\n");
    abort();
    return -1;
  }

  /// Collect all global regions.
  void getGlobalRegions(RegionInfo* regions, int* regionNumb) const {
    size_t index = 0;

    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;

      // skip libdoubletake
      if(m.isGlobals(_main_exe) && m.getFile().find("libdoubletake") == std::string::npos) {
        //PRINT("getGlobalRegiions: m.getBase() %lx m.getLimit() %lx isglobals and added\n", m.getBase(), m.getLimit());
        regions[index].start = m.getBase();
        regions[index].end = m.getLimit();
        index++;
      }
    }
    
    // We only need to return this.
    *regionNumb = index;
  }

private:
  std::map<interval, mapping, std::less<interval>,
           HL::STLAllocator<std::pair<interval, mapping>, InternalHeapAllocator>> _mappings;

  std::string _main_exe;
  void* _appTextStart;
  void* _appTextEnd;
  void* _doubletakeStart;
  void* _doubletakeEnd;
};

#endif
