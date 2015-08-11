#if !defined(DOUBLETAKE_XGLOBALS_H)
#define DOUBLETAKE_XGLOBALS_H

/*
 * @file   xglobals.h
 * @brief  Management of all globals, including the globals of applications, libc.so and
 * libpthread.so.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *         We adopted this from sheriff project, but we do substantial changes here.
 */

#include <stddef.h>
#include <stdint.h>

#include "selfmap.hh"
#include "xdefines.hh"
#include "xmapping.hh"

/// @class xglobals
/// @brief Maps the globals region onto a persistent store.
class xglobals {
public:
  xglobals() : _numbRegions(0) {
    // PRINF("GLOBALS_START is %x\n", GLOBALS_START);
  }

  void initialize() {
    // We check the mappings to find existing globals.

    // Trying to get information about different text segmensts.
    selfmap::getInstance().getTextRegions();

    // Trying to get the information of global regions
    selfmap::getInstance().getGlobalRegions(_gRegions, &_numbRegions);

    // Do the initialization for each global.
    for(int i = 0; i < _numbRegions; i++) {
     // fprintf(stderr, "Call begin at i %d from %p to %p\n", i, _gRegions[i].start, _gRegions[i].end);
      _maps[i].initialize(_gRegions[i].start,
                          (size_t)((intptr_t)_gRegions[i].end - (intptr_t)_gRegions[i].start));
    }

    // fprintf(stderr, "_numbRegions is %d\n", _numbRegions);
    // while(1);
  }

  void finalize() {
    // Nothing need to do here.
  }

  int getRegions() { return _numbRegions; }

  void getGlobalRegion(int index, unsigned long* begin, unsigned long* end) {
    if(index < _numbRegions) {
      *begin = (unsigned long)_gRegions[index].start;
      *end = (unsigned long)_gRegions[index].end;
    } else {
      *begin = 0;
      *end = 0;
    }
  }

  void recoverMemory() {
    for(int i = 0; i < _numbRegions; i++) {
      // PRINF("Call begin at i %d from %p to %p\n", i, _gRegions[i].start, _gRegions[i].end);
      _maps[i].recoverMemory(NULL);
    }
  }

  // Commit all regions in the end of each transaction.
  void backup() {
    for(int i = 0; i < _numbRegions; i++) {
      _maps[i].backup(NULL);
    }
  }

  void commit(void* start, size_t size, int index) { _maps[index].commit(start, size); }

private:
  int _numbRegions; // How many blocks actually.
  regioninfo _gRegions[xdefines::NUM_GLOBALS];
  xmapping _maps[xdefines::NUM_GLOBALS];
};

#endif
