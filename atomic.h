// -*- C++ -*-

/*
 Author: Emery Berger, http://www.cs.umass.edu/~emery
 
 Copyright (c) 2007-8 Emery Berger, University of Massachusetts Amherst.

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

/**
 * @class atomic
 * @brief A wrapper class for basic atomic hardware operations.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef SHERIFF_ATOMIC_H
#define SHERIFF_ATOMIC_H

class atomic {
public:

  inline static unsigned long exchange(volatile unsigned long * oldval,
      unsigned long newval) {
#if defined(X86_32BIT)
    asm volatile ("lock; xchgl %0, %1"
#else
    asm volatile ("lock; xchgq %0, %1"
#endif
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
    return newval;
  }

  // Atomic increment 1 and return the original value.
  static inline int increment_and_return(volatile unsigned long * obj) {
    int i = 1;
#if defined(X86_32BIT)
    asm volatile("lock; xaddl %0, %1"
#else
    asm volatile("lock; xadd %0, %1"
#endif
        : "+r" (i), "+m" (*obj)
        : : "memory");
    return i;
  }

  static inline void increment(volatile unsigned long * obj) {
    asm volatile("lock; incl %0"
        : "+m" (*obj)
        : : "memory");
  }

  static inline void add(int i, volatile unsigned long * obj) {
#if defined(X86_32BIT)
    asm volatile("lock; addl %0, %1"
#else
    asm volatile("lock; add %0, %1"
#endif
        : "+r" (i), "+m" (*obj)
        : : "memory");
  }

  static inline void decrement(volatile unsigned long * obj) {
    asm volatile("lock; decl %0;"
        : :"m" (*obj)
        : "memory");
  }

  // Atomic decrement 1 and return the original value.
  static inline int decrement_and_return(volatile unsigned long * obj) {
    int i = -1;
#if defined(X86_32BIT)
    asm volatile("lock; xaddl %0, %1"
#else
    asm volatile("lock; xadd %0, %1"
#endif
        : "+r" (i), "+m" (*obj)
        : : "memory");
    return i;
  }

  static inline void atomic_set(volatile unsigned long * oldval,
      unsigned long newval) {
#if defined(X86_32BIT)
    asm volatile ("lock; xchgl %0, %1"
#else
    asm volatile ("lock; xchg %0, %1"
#endif
        : "=r" (newval)
        : "m" (*oldval), "0" (newval)
        : "memory");
    return;
  }

  static inline int atomic_read(const volatile unsigned long *obj) {
    return (*obj);
  }

  static inline void memoryBarrier(void) {
    // Memory barrier: x86 only for now.
    __asm__ __volatile__ ("mfence": : :"memory");
  }

};

#endif
