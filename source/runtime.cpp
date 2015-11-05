/**
 * @file  runtime.cpp
 * @brief Global functions not tied to the global initialization of
 *        libdoubletake.
 */

#include "doubletake.hh"
#include "xrun.hh"

int doubletake::findStack(pid_t tid, uintptr_t *bottom, uintptr_t *top) {
  return xrun::getInstance().findStack(tid, bottom, top);
}

void doubletake::printStackCurrent() {
  return xrun::getInstance().printStackCurrent();
}

void doubletake::printStack(size_t len, void **frames) {
  return xrun::getInstance().printStack(len, frames);
}

bool doubletake::isLib(void *pcaddr) {
  return xrun::getInstance().isDoubleTake(pcaddr);
}
