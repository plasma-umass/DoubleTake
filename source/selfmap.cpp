/*
 * @file   selfmap.h
 * @brief  Analyze the self mapping.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "selfmap.hh"

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.hh"
#include "xdefines.hh"
#include "xthread.hh"

#define MAX_BUF_SIZE 4096

// Normally, callstack only saves next instruction address.
// To get current callstack, we should substract 1 here.
// Then addr2line can figure out which instruction correctly
#define PREV_INSTRUCTION_OFFSET 1

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

static bool isDoubleTake(const mapping &m) {
  return m.getFile().find("libdoubletake") != std::string::npos;
}

selfmap::selfmap()
  : _mappings(), _exe(""), _appTextStart(nullptr), _appTextEnd(nullptr),
    _doubletakeStart(nullptr), _doubletakeEnd(nullptr),
    _doubletakeMapped(false) {}

void selfmap::initialize() {
  char path[PATH_MAX];

  // readlink's result isn't necessarily null-terminated - record the
  // length of the result.
  ssize_t len = Real::readlink("/proc/self/exe", path, PATH_MAX-1);
  _exe = std::string(path, len); // XXX: allocates from tempmalloc?

  // Build the mappings data structure
  ifstream maps_file("/proc/self/maps");

  while(maps_file.good() && !maps_file.eof()) {
    mapping m;
    maps_file >> m;
    // FIXME: we get an invalid entry when parsing the [vsyscall]
    // entry on 64-bit systems, I think because the addresses are
    // quite large: ffffffffff600000.  Luckily, the vsyscall page is
    // the last one in /maps. We should fix that, and not just bail
    // out here.
    if(!m.valid()) {
      break;
    }
    _mappings[interval(m.getBase(), m.getLimit())] = m;

    // record information on the main executable and libdoubletake
    if(m.isText()) {
      if(::isDoubleTake(m)) {
        _doubletakeStart = (void*)m.getBase();
        _doubletakeEnd = (void*)m.getLimit();
        _doubletakeMapped = true;
      } else if(m.getFile() == _exe) {
        _appTextStart = (void*)m.getBase();
        _appTextEnd = (void*)m.getLimit();
      }
    }
  }
}

void selfmap::getGlobalRegions(RegionInfo *regions, int *count) const {
  size_t index = 0;

  for(const auto& entry : _mappings) {
    const mapping& m = entry.second;
    // skip libdoubletake
    if(m.isGlobals(_exe) && !::isDoubleTake(m)) {
      regions[index].start = m.getBase();
      regions[index].end = m.getLimit();
      index++;
    }
  }

  // We only need to return this.
  *count = index;
}

// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
void selfmap::printStackCurrent() {
  void* array[256];
  int frames;

  // get void*'s for all entries on the stack
  current->makeUnsafe();
  frames = backtrace(array, 256);
  //  write(2, "FD PRINT:{\n", 11);
  //  backtrace_symbols_fd(array, frames, 2);
	//char** syms = backtrace_symbols(array, frames);
  //  write(2, "}FD PRINT\n", 10);

  // Print out the source code information if it is an overflow site.
  this->printStack(frames, array);

  current->makeSafe();
}

// Calling system involves a lot of irrevocable system calls.
void selfmap::printStack(int frames, void** array) {
#if 0
  char** syms = backtrace_symbols(array, frames);
  
  for(int i=0; i<frames; i++) {
    fprintf(stderr, "  %d: %s\n", i, syms[i]);
  }
#endif

#if 1
  char buf[256];
  //  fprintf(stderr, "printCallStack(%d, %p)\n", depth, array);
  for(int i = 0; i < frames; i++) {
    void* addr = (void*)((unsigned long)array[i] - PREV_INSTRUCTION_OFFSET);
    if (isApplication(addr)) {
      //PRINT("\tcallstack frame %d: %p\t", i, addr);
      // Print out the corresponding source code information
      sprintf(buf, "addr2line -C -a -i -p -e %s %p", _exe.c_str(), (void *)addr);
      system(buf);
    }
  }
#endif

#if 0
  // We print out the first one who do not belong to library itself
  //else if(index == 1 && !isDoubleTakeLibrary((void *)addr)) {
  else if(!isDoubleTakeLibrary((void *)addr)) {
    index++;
    PRINT("\tcallstack frame %d: %p\n", index, (void *)addr);
  }
#endif
}
