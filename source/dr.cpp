/* The code is adopted from the GDB-7.5/gdb/i386-nat.c or i386-linux-nat.c, 
 * but we delete most of unrelated functions and change the names of functions.
 * Author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>  
 */
/* Native-dependent code for GNU/Linux i386.

   Copyright (C) 1999-2012 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "dr.h"
#include "log.h"

#undef offsetof
#define offsetof(type, member)   ((size_t)((char *)&(*(type *)0).member - \
                                           (char *)&(*(type *)0)))

/* Whether or not to print the mirrored debug registers.  */
static int print_dr = 0;

/* Types of operations supported by handle_nonaligned_watchpoint.  */
typedef enum { WP_INSERT, WP_REMOVE, WP_COUNT } wp_op_t;

/* Internal functions.  */

/* Return the value of a 4-bit field for DR7 suitable for watching a
   region of LEN bytes for accesses of type TYPE.  LEN is assumed to
   have the value of 1, 2, or 4.  */
static unsigned length_and_rw_bits (int len, enum target_hw_bp_type type);

/* Insert a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bit-field from DR7 which describes the length and
   access type of the region to be watched by this watchpoint.  Return
   0 on success, -1 on failure.  */
static int insert_aligned_watchpoint (struct debug_reg_state *state,
					   CORE_ADDR addr,
					   unsigned len_rw_bits);

/* Remove a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */
static int remove_aligned_watchpoint (struct debug_reg_state *state,
					   CORE_ADDR addr,
					   unsigned len_rw_bits);

/* Insert or remove a (possibly non-aligned) watchpoint, or count the
   number of debug registers required to watch a region at address
   ADDR whose length is LEN for accesses of type TYPE.  Return 0 on
   successful insertion or removal, a positive number when queried
   about the number of registers, or -1 on failure.  If WHAT is not a
   valid value, bombs through internal_error.  */
static int handle_nonaligned_watchpoint (struct debug_reg_state *state,
					      wp_op_t what,
					      CORE_ADDR addr, int len,
					      enum target_hw_bp_type type);

/* Per-inferior data key.  */
struct debug_reg_state state;

/* Get debug register REGNUM value from a specified process.  */
static unsigned long dr_get (pid_t pid, int regnum)
{
  unsigned long value;

  errno = 0;
  value = ptrace (PTRACE_PEEKUSER, pid, offsetof(struct user, u_debugreg[regnum]), 0);

  if (errno != 0) {
    PRWRN("Couldn't read debug register %d at pid %d\n", regnum, pid);
  }

  return value;
}

/* Set debug register REGNUM to VALUE in the specified process. */
static void dr_set (pid_t pid, int regnum, unsigned long value)
{
  errno = 0;
//  PRWRN("set debug register %d on pid %d, value %lx\n", regnum, pid, value); 
  ptrace (PTRACE_POKEUSER, pid, offsetof (struct user, u_debugreg[regnum]), value);
  if (errno != 0) {
    PRWRN("Couldn't set debug register %d value %lx error %s\n", regnum, value, strerror(errno));
  //  abort();
  }
  
  return;
}

/* Return the address of specified register. Not good for DR6 and DR7 */
static unsigned long dr_get_addr (pid_t pid, int regnum)
{
  assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);
  /* DR6 and DR7 are retrieved with some other way.  */
//  return dr_get (pid, regnum);
 // state.dr_mirror[i] = 0;
  return state.dr_mirror[regnum]; 
}

/* Return the inferior's DR7 debug control register.  */
static unsigned long dr_get_control (pid_t pid)
{
  return dr_get (pid, DR_CONTROL);
}

/* Get DR_STATUS from a specified process.  */
static unsigned long dr_get_status (pid_t pid)
{
  return dr_get (pid, DR_STATUS);
}

//#error "linux_nat_iterate_watchpoint_lwps is not here!!!"

/* Set DR_CONTROL to ADDR in all LWPs of the current inferior.  */
static void dr_set_control (pid_t pid, unsigned long control)
{
  //linux_nat_iterate_watchpoint_lwps (update_debug_registers_callback, NULL);
  dr_set (pid, DR_CONTROL, (unsigned long)control);
}

/* Set address REGNUM (zero based) to ADDR in all LWPs of the current
   inferior.  */

static void dr_set_addr (pid_t pid, int regnum, CORE_ADDR addr)
{
  assert (regnum >= DR_FIRSTADDR && regnum <= DR_LASTADDR);

  dr_set (pid, regnum, (unsigned long)addr);

  //linux_nat_iterate_watchpoint_lwps (update_debug_registers_callback, NULL);
}

/* Clear the reference counts and forget everything we knew about the
   debug registers.  */
void init_debug_registers()
{
  int i;

  // Cleanup all inferior registers.
  // These registers will be updated when we are adding/removing watchpoints
  ALL_DEBUG_ADDR_REGISTERS (i)
  {
     state.dr_mirror[i] = 0;
     state.dr_ref_count[i] = 0;
  }
  state.dr_control_mirror = 0;
  state.dr_status_mirror  = 0;
}


/* Per-inferior hook for register_inferior_data_with_cleanup.  */
/* Get debug registers state. */
//FIXME: maybe we have to use the per process related state.
struct debug_reg_state * debug_reg_state()
{
  return &state;
}


/* Implementation.  */

/* Print the values of the mirrored debug registers.  This is called
   when print_dr is non-zero.  
*/ 
static void print_debug_register (struct debug_reg_state *state,
	      const char *func, CORE_ADDR addr,
	      int len, int type)
{
  int addr_size = sizeof(CORE_ADDR);
  int i;

  if(addr || len) {
    printf (" (addr=%lx, len=%d, type=%s)",
		       (unsigned long)addr, len,
		       type == hw_write ? "data-write"
		       : (type == hw_read ? "data-read"
			  : (type == hw_access ? "data-read/write"
			     : (type == hw_execute ? "instruction-execute"
				: "??unknown??"))));
  }

  ALL_DEBUG_ADDR_REGISTERS(i)
  {
      printf ("\\tDR%d: ref.count=%d addr %p\n",
			        i, state->dr_ref_count[i], state->dr_mirror);
  }
}

/* Return the value of a 4-bit field for DR7 suitable for watching a
   region of LEN bytes for accesses of type TYPE.  LEN is assumed to
   have the value of 1, 2, or 4.  */
static unsigned dr_length_and_rw_bits (int len, int type)
{
  unsigned value = 0;

  switch (type)
  {
    case hw_execute:
	    value = DR_RW_EXECUTE;
	    break;

    case hw_write:
	    value = DR_RW_WRITE;
	    break;

    case hw_read:
	    fprintf (stderr, "The i386 doesn't support data-read watchpoints.\n");
      break;

    case hw_access:
	    value = DR_RW_READ;
	    break;
     
    default:
      PRWRN("Invalid watchpoint type %d in dr_length_and_rw_bits.\n", (int) type);
      break;
  }

  // Now we are trying to check the length bits.
  switch (len)
  {
    case 1:
	    value |= DR_LEN_1;
      break;


    case 2:
	    value |= DR_LEN_2;
      break;
      
    case 4:
	    value |= DR_LEN_4;
      break;
      
    case 8:
    {
      if (TARGET_HAS_DR_LEN_8) {
	      value |= DR_LEN_8;
        break;
      }
    }
	/* ELSE FALL THROUGH */
    default:
      PRWRN("Invalid watchpoint length %d in dr_length_and_rw_bits.\n", len);
      break;
    }
    return value;
}

/* Insert a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region to be watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */
// We actually didn't set actual registers here, only mark in local state. 
static int insert_aligned_watchpoint (struct debug_reg_state *state,
				CORE_ADDR addr, unsigned len_rw_bits)
{
  int i;

  /* First, look for an occupied debug register with the same address
     and the same RW and LEN definitions.  If we find one, we can
     reuse it for this watchpoint as well (and save a register).  */
  ALL_DEBUG_ADDR_REGISTERS(i)
  {
    // We will check the saved state, not the actual registers for performance reason
    if (!DR_VACANT (state, i)
	    && state->dr_mirror[i] == addr
	    && DR_GET_RW_LEN (state->dr_control_mirror, i) == len_rw_bits)
	  {
      // If everything is the same, we only increment the dr_ref_count
      // we don't need to actual do anything.
	    state->dr_ref_count[i]++;
	    return 0;
	  }
  }

  /* Next, look for a vacant debug register.  */
  ALL_DEBUG_ADDR_REGISTERS(i)
  {
    if (DR_VACANT (state, i))
	    break;
  }

  /* No more debug registers!  */
  if (i >= DR_NADDR)
    return -1;

  /* Now set up the register I to watch our region.  */

  /* Record the info in our local mirrored array.  */
  state->dr_mirror[i] = addr;
  state->dr_ref_count[i] = 1;
  DR_SET_RW_LEN (state, i, len_rw_bits);
  /* Note: we only enable the watchpoint locally, i.e. in the current
     task.  Currently, no i386 target allows or supports global
     watchpoints;
   */ 
  DR_LOCAL_ENABLE (state, i);
  state->dr_control_mirror |= DR_LOCAL_SLOWDOWN;
  state->dr_control_mirror &= DR_CONTROL_MASK;

  return 0;
}

/* Remove a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */
static int remove_aligned_watchpoint (struct debug_reg_state *state,
				   CORE_ADDR addr, unsigned len_rw_bits)
{
  int i, retval = -1;

  ALL_DEBUG_ADDR_REGISTERS(i)
  {
    if (!DR_VACANT (state, i)
	    && state->dr_mirror[i] == addr
	    && DR_GET_RW_LEN (state->dr_control_mirror, i) == len_rw_bits)
	  {
	    if (--state->dr_ref_count[i] == 0) /* no longer in use?  */
	    {
	      /* Reset our mirror.  */
	      state->dr_mirror[i] = 0;
	      DR_DISABLE (state, i);
	    }
	    retval = 0;
	  }
  }

  return retval;
}

/* Insert or remove a (possibly non-aligned) watchpoint, or count the
   number of debug registers required to watch a region at address
   ADDR whose length is LEN for accesses of type TYPE.  Return 0 on
   successful insertion or removal, a positive number when queried
   about the number of registers, or -1 on failure.  If WHAT is not a
   valid value, bombs through internal_error.  */

static int
handle_nonaligned_watchpoint (struct debug_reg_state *state,
				   wp_op_t what, CORE_ADDR addr, int len, enum target_hw_bp_type type)
{
  int retval = 0;
  int max_wp_len = TARGET_HAS_DR_LEN_8 ? 8 : 4;

  static int size_try_array[8][8] =
  {
    {1, 1, 1, 1, 1, 1, 1, 1},	/* Trying size one.  */
    {2, 1, 2, 1, 2, 1, 2, 1},	/* Trying size two.  */
    {2, 1, 2, 1, 2, 1, 2, 1},	/* Trying size three.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size four.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size five.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size six.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size seven.  */
    {8, 1, 2, 1, 4, 1, 2, 1},	/* Trying size eight.  */
  };

  while (len > 0)
  {
      int align = addr % max_wp_len;
    /* Four (eight on AMD64) is the maximum length a debug register can watch.  */
      int trylen = (len > max_wp_len ? (max_wp_len - 1) : len - 1);
      int size = size_try_array[trylen][align];

      if (what == WP_COUNT)
	    {
	  /* size_try_array[] is defined such that each iteration
	     through the loop is guaranteed to produce an address and a
	     size that can be watched with a single debug register.
	     Thus, for counting the registers required to watch a
	     region, we simply need to increment the count on each
	     iteration.  */
	      retval++;
	    } 
      else
	    { 
	      unsigned len_rw = dr_length_and_rw_bits (size, type);

	      if (what == WP_INSERT)
	        retval = insert_aligned_watchpoint (state, addr, len_rw);
	      else if (what == WP_REMOVE)
	        retval = remove_aligned_watchpoint (state, addr, len_rw);
	      else
            PRWRN("Invalid value %d handle_nonaligned_watchpoint.\n", (int)what);
	      if (retval)
	        break;
	    }

      addr += size;
      len -= size;
    }

    return retval;
}

/* Update the inferior's debug registers with the new debug registers
   state, in NEW_STATE, and then update our local mirror to match.  */
static void update_inferior_debug_regs (pid_t pid, struct debug_reg_state *new_state)
{
  struct debug_reg_state *state = debug_reg_state ();
  int i;

//  PRWRN("pid is %d\n", pid);

  ALL_DEBUG_ADDR_REGISTERS (i)
  {
    // Check whether we have changed the registers or not.
    // In fact, this logic is not good. If the existing state is vacant but the new state 
    // not vacant, then we should update the actual register
    if (DR_VACANT (new_state, i) != DR_VACANT (state, i)) {
	    assert(DR_VACANT(state, i) != 0);
      dr_set_addr (pid, i, new_state->dr_mirror[i]);
    }
    else {
	    assert (new_state->dr_mirror[i] == state->dr_mirror[i]);
    }
  }

  if (new_state->dr_control_mirror != state->dr_control_mirror)
    dr_set_control (pid, new_state->dr_control_mirror);

  // Update corresponding state to the new state
  *state = *new_state;
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */
int insert_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type, pid_t pid)
{
  struct debug_reg_state *state = debug_reg_state ();
  int retval;
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  struct debug_reg_state local_state = *state;

  if (type == hw_read)
    return 1; /* unsupported */

  if (((len != 1)
       && (len !=2)
       && (len !=4)
       && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || (addr % len != 0)) 
    {
      retval = handle_nonaligned_watchpoint (&local_state, WP_INSERT, addr, len, type);
    } else {
    unsigned len_rw = dr_length_and_rw_bits (len, type);
    
    retval = insert_aligned_watchpoint (&local_state, addr, len_rw);
  }

  // When we update the local state successfully, now it is time to 
  // update the actual debug registers.
  if (retval == 0)
    update_inferior_debug_regs (pid, &local_state);

  if (print_dr)
    print_debug_register (state, "insert_watchpoint", addr, len, type);

  return retval;
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */
int remove_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type, pid_t pid)
{
  struct debug_reg_state *state = debug_reg_state ();
  int retval;
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  struct debug_reg_state local_state = *state;

  if (((len != 1 && len !=2 && len !=4) && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || addr % len != 0) 
  {
    retval = handle_nonaligned_watchpoint (&local_state,
						WP_REMOVE, addr, len, type);
  }
  else
  {
      unsigned len_rw = dr_length_and_rw_bits (len, type);

      retval = remove_aligned_watchpoint (&local_state,
					       addr, len_rw);
    }

  if (retval == 0)
    update_inferior_debug_regs (pid, &local_state);

  if (print_dr)
    print_debug_register (state, "remove_watchpoint", addr, len, type);

  return retval;
}

/* If the inferior has some watchpoint that triggered, set the
   address associated with that watchpoint and return non-zero.
   Otherwise, return zero.  */
unsigned long get_watching_address (pid_t pid)
{
  struct debug_reg_state *state = debug_reg_state ();
  CORE_ADDR addr = 0;
  /* The current thread's DR_STATUS.  We always need to read this to
     check whether some watchpoint caused the trap.  */
  unsigned status;
  /* We need DR_CONTROL as well, but only iff DR_STATUS indicates a
     data breakpoint trap.  Only fetch it when necessary, to avoid an
     unnecessary extra syscall when no watchpoint triggered.  */
  int control_p = 0;
  unsigned control = 0;
  int i;

  // Get the status register to find out which watch point are trigged. 
  status = dr_get_status(pid);

  ALL_DEBUG_ADDR_REGISTERS(i)
  {
    // When a register is not hit, we check next one.
    if (!DR_WATCH_HIT (status, i))
	    continue;

#if 0
    // Why we need to check control register? 
    // FIXME: maybe we should improve this???? We don't need to check control register
    // and address register
     if (!control_p)
	  {
	    control = dr_get_control (pid);
	    control_p = 1;
	  }

    /* This second condition makes sure DRi is set up for a data
	     watchpoint, not a hardware breakpoint.
    */ 
    if(DR_GET_RW_LEN (control, i) != 0)
#endif
	  {
      // Why we need to check the actual register but not the 
      // global registers? Maybe in our situation, it is 
      // safe to use the saved registers???
      // Actually we read the corresponding debug register to find out 
      // what is the current address
	    addr = dr_get_addr (pid, i);
	    if (print_dr)
	      print_debug_register (state, "watchpoint_hit", addr, -1, hw_write);

      break;
	  }
  }

  if (print_dr && addr == 0)
    print_debug_register (state, "stopped_data_addr", 0, 0, hw_write);

  return addr;
}

unsigned long get_watching_status (pid_t pid, unsigned long * value)
{
  unsigned long addr;
  addr = get_watching_address(pid);

  // Now save the value of this address to the "value" pointer
  *value = ptrace (PTRACE_PEEKDATA, pid, addr, 0);
    
  return addr;
}

/* Insert a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, EBUSY on failure.  */
/* Insert a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, EBUSY on failure.  */
static int insert_hw_breakpoint (pid_t pid, CORE_ADDR watch_address)
{
  struct debug_reg_state *state = debug_reg_state ();
  unsigned len_rw = dr_length_and_rw_bits (1, hw_execute);
  CORE_ADDR addr = watch_address;
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  struct debug_reg_state local_state = *state;
  int retval = insert_aligned_watchpoint (&local_state,
					       addr, len_rw) ? EBUSY : 0;

  if (retval == 0)
    update_inferior_debug_regs (pid, &local_state);

  if (print_dr)
    print_debug_register (state, "insert_hwbp", addr, 1, hw_execute);

  return retval;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */
int remove_hw_breakpoint (pid_t pid, CORE_ADDR watch_address)
{
  struct debug_reg_state *state = debug_reg_state ();
  unsigned len_rw = dr_length_and_rw_bits (1, hw_execute);
  CORE_ADDR addr = watch_address;
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  // By doing this, we are getting a local copy of existing status.
  struct debug_reg_state local_state = *state;
  int retval = remove_aligned_watchpoint (&local_state,
					       addr, len_rw);

  if (retval == 0)
    update_inferior_debug_regs (pid, &local_state);

  if (print_dr)
    print_debug_register (state, "remove_hwbp", addr, 1, hw_execute);

  return retval;
}

void  resume_process (pid_t pid)
{
//  kill(pid, SIGSTOP);
  // Send corresponding child a SIGUSR2
 // if(ptrace (PTRACE_CONT, pid, 0, SIGCONT) == -1) {
  //if(ptrace (PTRACE_CONT, pid, 0, SIGUSR2) == -1) {
  if(ptrace (PTRACE_CONT, pid, 0, SIGCONT) == -1) {
    PRWRN("Can't start the child process.\n");
    abort();
  }
}

void  pass_signals (pid_t pid, int signal)
{
//  kill(pid, SIGSTOP);
  // Send corresponding child a SIGUSR2
  if(ptrace (PTRACE_CONT, pid, 0, signal) == -1) {
//  if(ptrace (PTRACE_CONT, pid, 0, signal) == -1) {
    PRWRN("Can't pass process %d a signal %d, the error %s\n", pid, signal, strerror(errno));
  //  abort();
  }
}

/* Set debug register REGNUM to VALUE in the specified process. */

// Get the data from the specified
unsigned long get_eip_addr(pid_t pid) {
  long value;
  unsigned long offset;

#if defined(__i386__)
offset = EIP * sizeof(unsigned long);
#elif defined(__x86_64__)
// copied from /usr/include/asm-x86_64/ptrace.h
#define RIP 128
offset = RIP;
#endif
  value = ptrace (PTRACE_PEEKUSER, pid, (void *)offset, NULL);

  return (unsigned long)value;
}
