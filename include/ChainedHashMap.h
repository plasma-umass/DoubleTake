#ifndef __CHAINED_HASHMAP__
#define __CHAINED_HASHMAP__

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

// The code is adapted from https://sites.google.com/site/cconcurrencypackage/hopscotch-hashing.
// The following is their original liscence. 
// However, we changed a little bit of their design.
// We remove the concept of segments in their code: each entry will
// have their code.

//------------------------------------------------------------------------------
// File    : ChainedHashMap.h
// Author  : Ms.Moran Tzafrir
// Written : 13 April 2009
// 
// Copyright (C) 2009 Moran Tzafrir, Nir Shavit.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License 
// along with this program; if not, write to the Free Software Foundation
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
// CLASS: ConcurrentHashMap
////////////////////////////////////////////////////////////////////////////////

#define MINIMUM_CAPACITY 		(32) 
#define MAXIMUM_CAPACITY 		(1 << 30) 
#define DEFAULT_LOAD_FACTOR   (3) 

template <class _tKey, 
			 class _tData,
			 class _tHash,
			 class _tLock,
			 class _tMemory>

class ChainedHashMap {
private:
	// INNER CLASSES ...........................................................
	struct Entry {
		unsigned int	volatile	_hash;
		_tKey				volatile	_key;
		Entry*			volatile	_next;
		_tData			volatile	_value;
	};

  // We are not using each entry based lock, however, we are using
  // the segment based lock.
	struct Segment {
		unsigned int volatile	_count;
		unsigned int volatile	_start_indx;
		_tLock _lock;

		void Lock() {
			_lock.lock();
		}
		void Unlock() {
			_lock.unlock();
		}

	};

	// PROPERTIES ..............................................................
	float				_loadFactor;
	int				_threshold;
	int volatile	_votesForResize;
	unsigned int	_freeSegSize;
	int				_bucketMask;

	unsigned int				_segmentShift;
	unsigned int volatile*	_iFree;
	Entry** volatile		   _table;
	unsigned int volatile	_tablesize;

	int				_segmentMask;
	Entry**			_freeList;
	Segment*			_segments;

	// UTILITIES ................................................................

	// Private Static Utilities .................................................
	inline static unsigned int NearestPowerOfTwo(const unsigned int _value) {
		unsigned int rc( 1 );
		while (rc < _value) {
			rc <<= 1;
		}
		return rc;
	}

	inline static unsigned int CalcDivideShift(const unsigned int _value) {
		unsigned int numShift( 0 );
		unsigned int curr( 1 );
		while (curr < _value) {
			curr <<= 1;
			++numShift;
		}
		return numShift;
	}

	static int bitcount(int w)
	{
		w -= (0xaaaaaaaa & w) >> 1;
		w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
		w = (w + (w >> 4)) & 0x0f0f0f0f;
		w += w >> 8;
		w += w >> 16;
		return w & 0xff;
	}

	static int p2capacity(int initialCapacity)
	{
		int cap = initialCapacity;

		// Compute the appropriate capacity
		int result;
		if (cap > MAXIMUM_CAPACITY || cap < 0) {
			result = MAXIMUM_CAPACITY;
		} else {
			result = MINIMUM_CAPACITY;
			while (result < cap)
				result <<= 1;
		}
		return result;
	}

	Entry** CreateNewTable(int capacity) {
		_threshold = (int)( (capacity * _loadFactor) / (_segmentMask+1)) + 1;
		//fprintf(stderr, "	CreateNewTable: capacity = %d, _threshold = %d\n", capacity, _threshold);
		Entry **newTable = (Entry **)_tMemory::byte_malloc( capacity * sizeof(Entry*) );
		memset(newTable, 0, capacity * sizeof(Entry *));
		return newTable;
	}

	inline Entry* GetNewEntry(const unsigned int iSegment) {
		if(0 != _freeList) {
			unsigned int numChecked( 0 );
			unsigned int iCheckFree( _iFree[iSegment] );
			unsigned int iFoundFree(0);
			bool isFound(false);
			do {
				if(0 == _freeList[iSegment][iCheckFree]._hash && 0 == _freeList[iSegment][iCheckFree]._key && 0 == _freeList[iSegment][iCheckFree]._value) {
					iFoundFree = iCheckFree;
					isFound = true;
				}

				++iCheckFree;
				if(iCheckFree >= _freeSegSize)
					iCheckFree = 0;
				++numChecked;
			} while( /*numChecked < 20 ||*/ !isFound);

			_iFree[iSegment] = iFoundFree;
			return &(_freeList[iSegment][iFoundFree]);
		}
		else {
			return (Entry*) _tMemory::byte_malloc(sizeof(Entry));
		}
	}

	inline void FreeEntry(Entry* ent) {
		if(0 == _freeList) {
			_tMemory::byte_free(ent);
		}
		else {
			ent->_hash  = 0;
			ent->_key   = 0;
			ent->_next  = 0;
			ent->_value = 0;
		}
	}

	void resize() {
		fprintf(stderr, "	resize\n");
		int i, j;

		Entry **assumedTable = _table;
		for (i = 0; i < (_segmentMask+1); ++i) {
			if (assumedTable != _table)
				break;
			_segments[i].Lock();
		}

		if (i == (_segmentMask+1))
			rehash();

		for (j = 0; j < i; ++j)
			_segments[j].Unlock();
	}

	void rehash() {
		fprintf(stderr, "	rehash\n");
		_votesForResize = 0; // reset

		Entry **oldTable = _table;
		int oldCapacity = _tablesize;

		if (oldCapacity >= MAXIMUM_CAPACITY) {
			_threshold = INT_MAX; // avoid re-triggering
			return;
		}

		int newCapacity = oldCapacity << 1;
		Entry **newTable = CreateNewTable(newCapacity);
		int mask = newCapacity - 1;

		for (int i = 0; i < oldCapacity ; i++) {
			// We need to guarantee that any existing reads of old Map can
			//  proceed. So we cannot yet null out each bin.  
			Entry *e = oldTable[i];

			if (e != NULL) {
				int idx = e->_hash & mask;
				Entry *_next = e->_next;

				//  Single node on list
				if (_next == NULL) {
					newTable[idx] = e;
					oldTable[i] = NULL;
				}
				else {    
					// Reuse trailing consecutive sequence of all same bit
					Entry *lastRun = e;
					int lastIdx = idx;
					for (Entry *last = _next; last != NULL; last = last->_next) {
						int k = last->_hash & mask;
						if (k != lastIdx) {
							lastIdx = k;
							lastRun = last;
						}
					}
					newTable[lastIdx] = lastRun;

					// Clone all remaining nodes
					Entry *beforep = NULL;
					Entry *newnode;
					for (Entry *p = e; p != lastRun; beforep = p, p = p->_next) {
						int k = p->_hash & mask;
						newnode = (Entry*) _tMemory::byte_malloc(sizeof(Entry));
						newnode->_key = p->_key;
						newnode->_next = newTable[k];
						newTable[k] = newnode;
					}
					if (beforep)
						beforep->_next = NULL;
					else
						oldTable[i] = NULL;
				}
			}
			Entry *head = oldTable[i];
			Entry *_next;
			while (head) {
				_next = head->_next;
				_tMemory::byte_free(head);
				head = _next;
			}
		}

		delete [] oldTable;
		_table = newTable;
		_tablesize = newCapacity;
	}

public:
	// Ctors ...................................................................
	ChainedHashMap(const int	initial_capacity	= 32*1024,	//Use the maximum number of keys you are going to use
						const int	concurrency_level	= 16,		//Number of updating threads
						float			loadFactor			= 0.75,
						bool			isPreAlloc			= false)
  : _segmentMask  ( NearestPowerOfTwo(concurrency_level) - 1),
    _segmentShift ( CalcDivideShift(NearestPowerOfTwo(initial_capacity/(NearestPowerOfTwo(concurrency_level)))-1) ),
		_bucketMask	  ( NearestPowerOfTwo(initial_capacity/(NearestPowerOfTwo(concurrency_level)))-1 )
	{
		_loadFactor = loadFactor;

    fprintf(stderr, "_setmentMask %d shift %d concurrentcy_level %d AAAA _bucketMask %d\n", _segmentMask, _segmentShift, concurrency_level, _bucketMask);
		_segments =  (Segment*) _tMemory::byte_malloc((_segmentMask+1)*sizeof(Segment));
		int curr_seg_start_indx=0;
		for (int i = 0; i <= _segmentMask; ++i) {
			_segments[i]._lock.init();
			_segments[i]._count = 0;
			_segments[i]._start_indx = curr_seg_start_indx;
			curr_seg_start_indx += (_bucketMask+1);
		}
		int cap = NearestPowerOfTwo(initial_capacity);
		_table = CreateNewTable(cap);
		_tablesize = cap;
		_votesForResize = 0;

		if(isPreAlloc) {
			_freeSegSize = (unsigned int) (((2*cap)/(_segmentMask+1)));
			_freeList	= (Entry**)_tMemory::byte_malloc((_segmentMask+1)*sizeof(Entry*));
			_iFree		= (unsigned int*)_tMemory::byte_malloc((_segmentMask+1)*sizeof(unsigned int));
			for (int iSeg(0); iSeg < (_segmentMask+1); ++iSeg) {
				_freeList[iSeg] = (Entry*)_tMemory::byte_malloc(_freeSegSize*sizeof(Entry));
				_iFree[iSeg] = 0;
				for (unsigned int iElm(0); iElm < _freeSegSize; ++iElm) {
					_freeList[iSeg][iElm]._hash  = 0;
					_freeList[iSeg][iElm]._key   = 0;
					_freeList[iSeg][iElm]._next  = 0;
					_freeList[iSeg][iElm]._value = 0;
				}
			}
		}
		else _freeList=0;
	}

	~ChainedHashMap() {
		clear();
		for (int i = 0; i <= _segmentMask; ++i) {
			_segments[i]._lock.init();
		}
		_tMemory::byte_free(_segments);
		_tMemory::byte_free(_table);
		if(0 != _freeList) {
			for (int i(0); i < (_segmentMask+1); ++i) {
				_tMemory::byte_free(_freeList[i]);
				_iFree[i] = 0;
			}
			_tMemory::byte_free(_freeList);
		}
	}

	// Query Operations ........................................................
	inline bool containsKey(const _tKey& key) {
		const unsigned int hkey( _tHash::Calc(key) ); 

		// Try first without locking...
		const unsigned int iSegment(( hkey >> _segmentShift) & _segmentMask);
	//	Segment& segment( _segments[iSegment] );
		Entry *first = _table[ (hkey & _bucketMask) + _segments[iSegment]._start_indx];
		Entry *e;

		for (e = first; e != NULL; e = e->_next) {
			if (hkey == e->_hash && e->_key == key) 
				return true;
		}
		return false;
	}

  inline _tData get(const _tKey& key) {
    const unsigned int hkey( _tHash::Calc(key) );

    // Try first without locking...
    const unsigned int iSegment(( hkey >> _segmentShift) & _segmentMask);
  //  Segment& segment( _segments[iSegment] );
    Entry *first = _table[ (hkey & _bucketMask) + _segments[iSegment]._start_indx];
    Entry *e;

    for (e = first; e != NULL; e = e->_next) {
      if (hkey == e->_hash && e->_key == key)
        return _tData(e->_value);;
    }
    return NULL;
  }

	// Modification Operations .................................................
	inline _tData putIfAbsent(const _tKey& key,  const _tData& data) {
		const unsigned int hkey( _tHash::Calc(key) ); 
		const unsigned int iSegment(( hkey >> _segmentShift) & _segmentMask);
		Segment& segment( _segments[iSegment] );
		segment.Lock();

    fprintf(stderr, "putIfAbsent, _segmentShift %d mask %d, hkey %d, iSegment %d\n", _segmentShift, _segmentMask, hkey, iSegment);
		int segcount;
		int votes;
		int index = (hkey & _bucketMask) + _segments[iSegment]._start_indx;
		Entry *first = _table[index];

		for (Entry *e = first; e != NULL; e = e->_next) {
			if (hkey == e->_hash && e->_key == key) {
				_tData rc(e->_value);
				segment.Unlock();
				return rc;
			}
		}

		// Add to front of list
		Entry *newEntry = GetNewEntry(iSegment);
		newEntry->_hash = hkey;
		newEntry->_key = key;
		newEntry->_value = data;
		newEntry->_next = first;
		_table[index] = newEntry;

		if ((segcount = ++_segments[iSegment]._count) < _threshold) {
			segment.Unlock();
			return (_tData)NULL;
		}

		int bit = (1 << (hkey & _segmentMask));
		votes = _votesForResize;
		if ((votes & bit) == 0)
			votes = _votesForResize |= bit;

		segment.Unlock();

		// Attempt resize if 1/4 segments vote,
		// or if this segment itself reaches the overall _threshold.
		// (The latter check is just a safeguard to avoid pathological cases.)
		if (bitcount(votes) >= ((_segmentMask+1) / 4)  || segcount > (_threshold * (_segmentMask+1))) 
			resize();

	  return (_tData)NULL;
	}

	inline _tData remove(const _tKey& key) {
		const unsigned int hkey( _tHash::Calc(key) ); 
		const unsigned int  iSegment  (( hkey >> _segmentShift) & _segmentMask);
		Segment&            segment   ( _segments[iSegment] );
		segment.Lock();

		int index = (hkey & _bucketMask) + _segments[iSegment]._start_indx;
		Entry *first = _table[index];
		Entry *e = first;

		for (;;) {
			if (e == NULL) {
				segment.Unlock();
				return (_tData)NULL;
			}
			if (hkey == e->_hash && e->_key == key)
				break;
			e = e->_next;
		}

		// All entries following removed node can stay in list, 
		// but all preceding ones need to be cloned.
		_tData rc(e->_value);
		Entry* newFirst = e->_next;
		Entry* needDel[16];
		int	   numNeedDel(1);
		needDel[0] = e;
		for (Entry* p = first; p != e; ) {
			Entry* newEntry		= GetNewEntry(iSegment);
			newEntry->_hash		= p->_hash;
			newEntry->_key			= p->_key;
			newEntry->_value		= p->_value;
			newEntry->_next		= newFirst;
			newFirst = newEntry;
			needDel[numNeedDel++] = p;
			p = p->_next;
		}
		_table[index] = newFirst;
		--(_segments[iSegment]._count);
		//
		// IMPORTANT: here we need to ALIGNED_FREE "e", but there is no memory management
		//
		for (int i(0); i<numNeedDel; ++i) {
			FreeEntry(needDel[i]);
		}
		segment.Unlock();
		return rc;
	}

	// .........................................................................
	void clear() {
		for (unsigned int iElm(0); iElm <_tablesize; ++iElm) {
			Entry* elm = _table[iElm];
			while(NULL != elm) {
				Entry* nextElm = elm->_next;
				FreeEntry(elm);
				elm = nextElm ;
			}
		}

		for (int i(0); i < (_segmentMask+1); ++i) {
			_segments[i]._lock.init();
			_segments[i]._count = 0;
		}

		memset(_table, 0, _tablesize * sizeof(Entry *));
	}

	unsigned int size() {
		int c = 0;
		for (int i = 0; i < (_segmentMask+1); ++i) {
			c += _segments[i]._count;
			fprintf(stderr, "	size seg %d, %d\n", i, _segments[i]._count);
		}
		return c;
	}

	bool isEmpty()	{
		for (int i = 0; i < (_segmentMask+1); ++i) 
			if (_segments[i]._count != 0)
				return false;
		return true;
	}

};

#endif
