// -*- C++ -*-

/*
  Copyright (c) 2012, University of Massachusetts Amherst.

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

#ifndef _XCONTEXT_H_
#define _XCONTEXT_H_

#include <signal.h>
#include <stdio.h>

#include <ucontext.h>

#include "selfmap.h"

/**
 * @class xcontext
 * @brief User context to support the rollback mechanism. 
 *
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

class xcontext {
public:
  
  xcontext() {
  }

  /// @brief Initialize this with the highest pointer possible on the stack.
  void initialize (void) {
    // First, we must get the stack corresponding information.
    selfmap::getStackInformation(&_stackBottom, &_stackTop);  

    // Never need to check the stack beyond this.
    _maxStackSize = _stackBottom - _stackTop;
  
    // Calculate the bottom of temporary stack.
    _tempStackBottom = (intptr_t)&_tempstack + xdefines::TEMP_STACK_SIZE;
 
//    fprintf(stderr, "stackbottom 0x%lx stacktop 0x%lx, _tempstack address %p to 0x%lx\n", _stackBottom, _stackTop,  &_tempstack, _tempStackBottom);
//    while (1);
  }


  /// @brief Save current calling context (i.e., current continuation).
  void saveContext (void) {
    volatile unsigned long tos;

    // Now, save the stack. FIXME...
    // Actually, we may not save all stack from _stackBottom to current place.
    // Then we can save some time to save this stack.
    saveStack((unsigned long *)_stackBottom, (unsigned long *) &tos);
   
    // Now, save the context.
    getcontext(&_context);
    // When we rollback, we will return here.
  }

  /// @brief Restore the previously saved context.
  void rollback (void) {
    /** There are two steps for this function.
     * First, we must recover the stack.
     * Second, we will setcontext to the saved context.
     * We should be careful about this function:
     *  a. If the saved stack is equal to or larger than current stack size, 
     *     then we can't simply overlap current stack from the saved stack.
     *     since it will screw current stack. 
     *  b. If the saved stack is smaller than current stacksize, then it can be
     *     safe to overlap current stack. 
     * In our implementation, we may utilize a temporary stack to recover the stack.
     */
    unsigned long ebp, esp;

    // The offset to the stack bottom.
    unsigned long espoffset, ebpoffset;

    unsigned long newebp, newesp;
    unsigned long stackStart;
    unsigned long offset;

    // Get current esp and ebp
#if defined(X86_32BIT)
    asm volatile(
      "movl %%ebp,%0\n" \
      "movl %%esp,%1\n" \
      :"=r"(ebp), "=r"(esp)
    );
#else
    asm volatile(
      "movq %%rbp,%0\n" \
      "movq %%rsp, %1\n" \
      :"=r"(ebp), "=r"(esp)
    );
#endif

    // Calculate the offset to stack bottom for ebp and esp register. 
    // Since we know that we are still using the original stack.
    espoffset = _stackBottom - esp;
    ebpoffset = _stackBottom - ebp;

    // Check whether we can utilize the temporary stack.
    if(espoffset > xdefines::TEMP_STACK_SIZE) {
      fprintf(stderr, "Now we can't use the reserved temporary stack, espoffset %lx temporary stack size %lx\n", xdefines::TEMP_STACK_SIZE, espoffset);
      // FIXME: we might use some malloced memory, but not now.
      abort();
    }


//    fprintf(stderr, "%d: ebp %lx esp %lx espofset %lx\n", getpid(), ebp, esp, espoffset);
    // Calculate the new ebp and esp registers. 
    // We will set ebp to the bottom of temporary stack. 
    newebp = _tempStackBottom - ebpoffset;
    newesp = _tempStackBottom - espoffset;

//    fprintf(stderr, "%d: before swtiching stack,  new ebp %lx esp %lx espofset %lx\n", getpid(), newebp, newesp, espoffset);

    // Copy the existing stack to the temporary stack.
    // Otherwise, we can not locate those global variables???
    memcpy((void *)newesp, (void *)esp, espoffset);

    // Switch the stack manually. 
    // It is important to switch in this place (not using a function call), otherwise, the lowest
    // level of frame will be poped out and the stack will return back to the original one
    // Then techniquely we cann't switch successfully. 
    // What we want is that new frames are using the new stack, but we will recover
    // the stack in the same function later to void problems!!!
#if defined(X86_32BIT)
    asm volatile(
      // Set ebp and esp to new pointer
      "movl %0, %%ebp\n"
      "movl %1, %%esp\n"
      : : "r" (newebp), "r" (newesp)
    );
#else
    asm volatile(
      // Set ebp and esp to new pointer
      "movq %0,%%rbp\n"
      "movq %1,%%rsp\n"
      : : "r" (newebp), "r" (newesp)
    );
#endif 

 //   fprintf(stderr, "%d: new ebp %lx esp %lx espofset %lx\n", getpid(), newebp, newesp, espoffset);
 //   fprintf(stderr, "%d: after switch\n", getpid());
 //   fprintf(stderr, "%d: _pstackTopSaved %p _stack %p _stacksize %lx\n", getpid(), _pstackTopSaved, &_stack, _stackSize);
    //fprintf(stderr, "%d: _pstackTopSaved %p _stack %p _stacksize %lx\n", getpid(), _pstackTopSaved, &_stack, _stackSize);
    // Now we are running in a different stack now.  
    // The recovery of stack are safe no matter how large of the original stack
    memcpy(_pstackTopSaved, &_stack, _stackSize);

    // After recovery of the stack, we can call setcontext to switch to original stack. 
    setcontext (&_context);
  } 

private:

  // Save the stack locally using the memcpy
  void saveStack (unsigned long *pbos, unsigned long *ptos) {
    _pstackTopSaved = ptos;
    _stackSize = (intptr_t)pbos - (intptr_t)ptos;

    // Using memcpy to save the stack since it is faster.  
    memcpy(_stack, _pstackTopSaved, _stackSize);
/*
    int i;
    n = pbos-ptos;
    _stackSize = n;
  
    for (i = 0; i < n; ++i) 
    {
      _stack[i] = pbos[-i];
   }
*/
  }

  /// A pointer to the base (highest address) of the stack.
  unsigned long _stackBottom;
  unsigned long _stackTop;
  
  // Saved stack top
  void * _pstackTopSaved;

  /// The saved registers, etc.
  ucontext_t _context;

  /// Current saved stack size.
  int _stackSize;
  int _maxStackSize;

  /// The saved stack contents, now it is inside the globals of library.
  char _stack[xdefines::MAX_STACK_SIZE];
  char _tempstack[xdefines::TEMP_STACK_SIZE];
  unsigned long _tempStackBottom;
};
#endif
