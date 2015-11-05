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

selfmap::selfmap() {
  // Read the name of the main executable
  // char buffer[PATH_MAX];
  //Real::readlink("/proc/self/exe", buffer, PATH_MAX);
  //_main_exe = std::string(buffer);
  bool gotMainExe = false;
  // Build the mappings data structure
  ifstream maps_file("/proc/self/maps");

  while(maps_file.good() && !maps_file.eof()) {
    mapping m;
    maps_file >> m;
    // It is more clean that that of using readlink. 
    // readlink will have some additional bytes after the executable file 
    // if there are parameters.	
    if(!gotMainExe) {
      _main_exe = std::string(m.getFile());
      gotMainExe = true;
    } 

    if(m.valid()) {
			//	fprintf(stderr, "Base %lx limit %lx\n", m.getBase(), m.getLimit()); 
      _mappings[interval(m.getBase(), m.getLimit())] = m;
    }
  }
}

// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
void selfmap::printStackCurrent() {
  void* array[256];
  int frames;

  // get void*'s for all entries on the stack
  xthread::disableCheck();
  frames = backtrace(array, 256);
  //  write(2, "FD PRINT:{\n", 11);
  //  backtrace_symbols_fd(array, frames, 2);
	//char** syms = backtrace_symbols(array, frames);
  //  write(2, "}FD PRINT\n", 10);

  // Print out the source code information if it is an overflow site.
  this->printStack(frames, array);

  xthread::enableCheck();
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
      sprintf(buf, "addr2line -C -a -i -p -e %s %p", _main_exe.c_str(), (void *)addr);
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
