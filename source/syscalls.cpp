#include "syscalls.hh"
#include "xrun.hh"

// Currently, epochBegin() will call xrun::epochBegin().
void syscalls::epochBegin() { xrun::getInstance().epochBegin(); }

void syscalls::epochEnd() {
  // PRINF("$$$$$$epochEnd at line %d\n", __LINE__);
  // PRINF("$$$$$$epochEnd at line %d$$$$$$$$$$$$$$$\n", __LINE__);
  // printf("$$$$$$epochEnd at line %d$$$$$$$$$$$$$$$\n", __LINE__);
  xrun::getInstance().epochEnd(false);
}

// Simply commit specified memory block
void syscalls::atomicCommit(void* addr, size_t size) {
  xrun::getInstance().atomicCommit(addr, size);
}
