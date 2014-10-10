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

#include "interval.hh"
#include "mm.hh"
#include "real.hh"

// From heaplayers
#include "wrappers/stlallocator.h"

using namespace std;

struct regioninfo {
  void* start;
  void* end;
};

/**
 * A single mapping parsed from the /proc/self/maps file
 */
class mapping {
public:
  mapping() : _valid(false) {}

  mapping(uintptr_t base, uintptr_t limit, char* perms, size_t offset, std::string file)
      : _valid(true), _base(base), _limit(limit), _readable(perms[0] == 'r'),
        _writable(perms[1] == 'w'), _executable(perms[2] == 'x'), _copy_on_write(perms[3] == 'p'),
        _offset(offset), _file(file) {}

  bool valid() const { return _valid; }

  bool isText() const { return _readable && !_writable && !_executable; }

  bool isStack() const { return _file == "[stack]"; }

  bool isGlobals() const {
    return _file.find("/") == 0 && _readable && _writable && !_executable && _copy_on_write;
  }

  uintptr_t getBase() const { return _base; }

  uintptr_t getLimit() const { return _limit; }

  const std::string& getFile() const { return _file; }

private:
  bool _valid;
  uintptr_t _base;
  uintptr_t _limit;
  size_t _offset;
  std::string _file;
  bool _readable;
  bool _writable;
  bool _executable;
  bool _copy_on_write;
};

/// Read a mapping from a file input stream
static std::ifstream& operator>>(std::ifstream& f, mapping& m) {
  if(f.good() && !f.eof()) {
    uintptr_t base, limit;
    char perms[5];
    size_t offset;
    size_t dev_major, dev_minor;
    int inode;
    string path;

    // Skip over whitespace
    f >> std::skipws;

    // Read in "<base>-<limit> <perms> <offset> <dev_major>:<dev_minor> <inode>"
    f >> std::hex >> base;
    if(f.get() != '-')
      return f;
    f >> std::hex >> limit;

    if(f.get() != ' ')
      return f;
    f.get(perms, 5);

    f >> std::hex >> offset;
    f >> std::hex >> dev_major;
    if(f.get() != ':')
      return f;
    f >> std::hex >> dev_minor;
    f >> std::dec >> inode;

    // Skip over spaces and tabs
    while(f.peek() == ' ' || f.peek() == '\t') {
      f.ignore(1);
    }

    // Read out the mapped file's path
    getline(f, path);

    m = mapping(base, limit, perms, offset, path);
  }

  return f;
}

class selfmap {
public:
  static selfmap& getInstance() {
    static char buf[sizeof(selfmap)];
    static selfmap* theOneTrueObject = new (buf) selfmap();
    return *theOneTrueObject;
  }

  /// Check whether an address is inside the DoubleTake library itself.
  bool isDoubleTakeLibrary(void* pcaddr) {
    return ((pcaddr >= _doubletakeStart) && (pcaddr <= _doubletakeEnd));
  }

  /// Check whether an address is inside the main application.
  bool isApplication(void* pcaddr) {
    return ((pcaddr >= _appTextStart) && (pcaddr <= _appTextEnd));
  }

  // Print out the code information about an eip address.
  // Also try to print out the stack trace of given pcaddr.
  void printCallStack();
  void printCallStack(int depth, void** array);
  static int getCallStack(void** array);

  void getStackInformation(void** stackBottom, void** stackTop) {
    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;
      if(m.isStack()) {
        *stackBottom = (void*)m.getBase();
        *stackTop = (void*)m.getLimit();
        return;
      }
    }
    fprintf(stderr, "Couldn't find stack mapping. Giving up.\n");
    abort();
  }

  /// Get information about global regions.
  void getTextRegions() {
    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;
      if(m.isText()) {
        if(m.getFile().find("libdoubletake") != std::string::npos) {
          _doubletakeStart = (void*)m.getBase();
          _doubletakeEnd = (void*)m.getLimit();

        } else if(m.getFile() == _main_exe) {
          _appTextStart = (void*)m.getBase();
          _appTextEnd = (void*)m.getLimit();
        }
      }
    }
  }

  /// Collect all global regions.
  void getGlobalRegions(regioninfo* regions, int* regionNumb) {
    size_t index = 0;

    for(const auto& entry : _mappings) {
      const mapping& m = entry.second;

      if(m.isGlobals()) {
        if(m.getFile() == _main_exe || m.getFile().find("libc-") != std::string::npos ||
           m.getFile().find("libstdc++") != std::string::npos ||
           m.getFile().find("libpthread") != std::string::npos) {

          regions[index].start = (void*)m.getBase();
          regions[index].end = (void*)m.getLimit();
          index++;
        }
      }

      *regionNumb = index;
    }
  }

private:
  selfmap() {
    // Read the name of the main executable
    char buffer[PATH_MAX];
    Real::readlink("/prof/self/exe", buffer, PATH_MAX);
    _main_exe = std::string(buffer);

    // Build the mappings data structure
    ifstream maps_file("/proc/self/maps");

    while(maps_file.good() && !maps_file.eof()) {
      mapping m;
      maps_file >> m;
      if(m.valid()) {
        _mappings[interval(m.getBase(), m.getLimit())] = m;
      }
    }
  }

  std::map<interval, mapping, std::less<interval>,
           HL::STLAllocator<std::pair<interval, mapping>, InternalHeapAllocator>> _mappings;

  std::string _main_exe;
  void* _appTextStart;
  void* _appTextEnd;
  void* _doubletakeStart;
  void* _doubletakeEnd;
};

#endif
