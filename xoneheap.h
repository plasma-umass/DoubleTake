// -*- C++ -*-

/*
  Copyright (c) 2008-12, University of Massachusetts Amherst.

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

#ifndef _XONEHEAP_H_
#define _XONEHEAP_H_

/**
 * @class xoneheap
 * @brief Wraps a single heap instance.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

template <class SourceHeap>
class xoneheap {
public:

  void initialize(void * ptr, size_t size) { getHeap()->initialize(ptr, size); }
  void sanitycheckInitialize(void * ptr, size_t size) { getHeap()->sanitycheckInitialize(ptr, size); }
  void finalize () { getHeap()->finalize(); }
  void begin () { getHeap()->begin(); }
  void commit (void) { getHeap()->commit(); }

  /// Check the buffer overflow.
  bool sanitycheckPerform() { return getHeap()->sanitycheckPerform(); }

  void stats () { getHeap()->stats(); }

  void openProtection () { getHeap()->openProtection(); }
  void closeProtection() { getHeap()->closeProtection(); }

  // Handling those metadata for rollback purpose 
  void recoverHeapMetadata () { getHeap()->recoverHeapMetadata(); }
  void saveHeapMetadata() { getHeap()->saveHeapMetadata(); }

  // Get heap start and end, this is used to check range.
  void * getHeapStart(void) { getHeap()->getHeapStart(); }
  void * getHeapEnd(void) { getHeap()->getHeapEnd(); }
  void * getHeapPosition(void) { getHeap()->getHeapPosition(); }
  
  // Now we don't support inRange anymore since it will be handled in the first level. 
  //bool inRange (void * ptr) { return getHeap()->inRange(ptr); }
  void handleWrite (void * ptr) { getHeap()->handleWrite(ptr); }

  void * malloc (size_t sz) { return getHeap()->malloc(sz); }
  void free (void * ptr) { getHeap()->free(ptr); }
  size_t getSize (void * ptr) { return getHeap()->getSize(ptr); }

  void sharemem_write_word(void * dest, unsigned long val) {
    getHeap()->sharemem_write_word(dest, val);
  }

  unsigned sharemem_read_word(void * dest) {
    return getHeap()->sharemem_read_word(dest);
  }

private:

  SourceHeap * getHeap (void) {
    static char heapbuf[sizeof(SourceHeap)];
    static SourceHeap * _heap = new (heapbuf) SourceHeap;
 //   fprintf (stderr, "xoneheap at %p\n", _heap);
    return _heap;
  }

};


#endif // _XONEHEAP_H_
