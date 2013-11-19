#ifndef __DR_H__
#define __DR_H__

#include <signal.h>
#include <syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <ucontext.h>
#include <assert.h>
#include <errno.h>

typedef unsigned long CORE_ADDR;

/* The code is adopted from the GDB-7.5/gdb/i386-nat.h or others, 
 * but we delete most of unrelated functions and change the names of functions.
 * Author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>  
 */

/* Debug registers' indices.  */
#define DR_FIRSTADDR 0
#define DR_LASTADDR  3
#define DR_NADDR     4  /* The number of debug address registers.  */
#define DR_STATUS    6  /* Index of debug status register (DR6).  */
#define DR_CONTROL   7  /* Index of debug control register (DR7).  */

#define ALL_DEBUG_ADDR_REGISTERS(i)  for (i = DR_FIRSTADDR; i < DR_LASTADDR; i++)

/* Support for hardware watchpoints and breakpoints using the i386
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   The functions below implement debug registers sharing by reference
   counts, and allow to watch regions up to 16 bytes long.  */

/* Support for 8-byte wide hw watchpoints.  */
#ifdef X86_32BIT
  #define TARGET_HAS_DR_LEN_8 0
#else
  #define TARGET_HAS_DR_LEN_8 1
#endif 

/* DR7 Debug Control register fields.  */

/* How many bits to skip in DR7 to get to R/W and LEN fields.  */
#define DR_CONTROL_SHIFT	16

/* How many bits in DR7 per R/W and LEN field for each watchpoint.  */
#define DR_CONTROL_SIZE		4

/* Watchpoint/breakpoint read/write fields in DR7.  */
#define DR_RW_EXECUTE	(0x0)	/* Break on instruction execution.  */
#define DR_RW_WRITE	(0x1)	/* Break on data writes.  */
#define DR_RW_READ	(0x3)	/* Break on data reads or writes.  */

/* This is here for completeness.  No platform supports this
   functionality yet (as of March 2001).  Note that the DE flag in the
   CR4 register needs to be set to support this.  */
#ifndef DR_RW_IORW
#define DR_RW_IORW	(0x2)	/* Break on I/O reads or writes.  */
#endif

/* Watchpoint/breakpoint length fields in DR7.  The 2-bit left shift
   is so we could OR this with the read/write field defined above.  */
#define DR_LEN_1	(0x0 << 2) /* 1-byte region watch or breakpoint.  */
#define DR_LEN_2	(0x1 << 2) /* 2-byte region watch.  */
#define DR_LEN_4	(0x3 << 2) /* 4-byte region watch.  */
#define DR_LEN_8	(0x2 << 2) /* 8-byte region watch (AMD64).  */

/* Local and Global Enable flags in DR7.

   When the Local Enable flag is set, the breakpoint/watchpoint is
   enabled only for the current task; the processor automatically
   clears this flag on every task switch.  When the Global Enable flag
   is set, the breakpoint/watchpoint is enabled for all tasks; the
   processor never clears this flag.

   Currently, all watchpoint are locally enabled.  If you need to
   enable them globally, read the comment which pertains to this in
   i386_insert_aligned_watchpoint below.  */
#define DR_LOCAL_ENABLE_SHIFT	0 /* Extra shift to the local enable bit.  */
#define DR_GLOBAL_ENABLE_SHIFT	1 /* Extra shift to the global enable bit.  */
#define DR_ENABLE_SIZE		2 /* Two enable bits per debug register.  */

/* Local and global exact breakpoint enable flags (a.k.a. slowdown
   flags).  These are only required on i386, to allow detection of the
   exact instruction which caused a watchpoint to break; i486 and
   later processors do that automatically.  We set these flags for
   backwards compatibility.  */
#define DR_LOCAL_SLOWDOWN	(0x100)
#define DR_GLOBAL_SLOWDOWN     	(0x200)

/* Fields reserved by Intel.  This includes the GD (General Detect
   Enable) flag, which causes a debug exception to be generated when a
   MOV instruction accesses one of the debug registers.
   FIXME: My Intel manual says we should use 0xF800, not 0xFC00.  */
#define DR_CONTROL_RESERVED	(0xFC00)

/* Auxiliary helper macros.  */

/* A value that masks all fields in DR7 that are reserved by Intel.  */
#define DR_CONTROL_MASK	(~DR_CONTROL_RESERVED)

/* The I'th debug register is vacant if its Local and Global Enable
   bits are reset in the Debug Control register.  */
#define DR_VACANT(state, i)					\
  (((state)->dr_control_mirror & (3 << (DR_ENABLE_SIZE * (i)))) == 0)

/* Locally enable the break/watchpoint in the I'th debug register.  */
#define DR_LOCAL_ENABLE(state, i) \
  do { \
    (state)->dr_control_mirror |= \
      (1 << (DR_LOCAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Globally enable the break/watchpoint in the I'th debug register.  */
#define DR_GLOBAL_ENABLE(state, i) \
  do { \
    (state)->dr_control_mirror |= \
      (1 << (DR_GLOBAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Disable the break/watchpoint in the I'th debug register.  */
#define DR_DISABLE(state, i) \
  do { \
    (state)->dr_control_mirror &= \
      ~(3 << (DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Set in DR7 the RW and LEN fields for the I'th debug register.  */
#define DR_SET_RW_LEN(state, i, rwlen) \
  do { \
    (state)->dr_control_mirror &= \
      ~(0x0f << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))); \
    (state)->dr_control_mirror |= \
      ((rwlen) << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))); \
  } while (0)

/* Get from DR7 the RW and LEN fields for the I'th debug register.  */
#define DR_GET_RW_LEN(dr7, i) \
  (((dr7) \
    >> (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))) & 0x0f)

/* Mask that this I'th watchpoint has triggered.  */
#define DR_WATCH_MASK(i)	(1 << (i))

/* Did the watchpoint whose address is in the I'th register break?  */
#define DR_WATCH_HIT(dr6, i) ((dr6) & (1 << (i)))


enum target_hw_bp_type
{
  hw_write   = 0,   /* Common  HW watchpoint */
  hw_read    = 1,   /* Read    HW watchpoint */
  hw_access  = 2,   /* Access  HW watchpoint */
  hw_execute = 3    /* Execute HW breakpoint */
};

/* Support for hardware watchpoints and breakpoints using the i386
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   These functions are:

      set_control              -- set the debug control (DR7)
				  register to a given value for all LWPs

      set_addr                 -- put an address into one debug
				  register for all LWPs

      get_addr                 -- return the address in a given debug
				  register of the current LWP

      get_status               -- return the value of the debug
				  status (DR6) register for current LWP

      get_control               -- return the value of the debug
				  control (DR7) register for current LWP

   Additionally, the native file should set the debug_register_length
   field to 4 or 8 depending on the number of bytes used for
   deubg registers.  */

/* Global state needed to track h/w watchpoints.  */
struct debug_reg_state
{
  /* Mirror the inferior's DRi registers.  We keep the status and
     control registers separated because they don't hold addresses.
     Note that since we can change these mirrors while threads are
     running, we never trust them to explain a cause of a trap.
     For that, we need to peek directly in the inferior registers.  */
  CORE_ADDR dr_mirror[DR_NADDR];

  /* Reference counts for each debug register.  */
  int dr_ref_count[DR_NADDR];
  
  unsigned  dr_status_mirror, dr_control_mirror;
};

/* Use this function to set i386_dr_low debug_register_length field
   rather than setting it directly to check that the length is only
   set once.  It also enables the 'maint set/show show-debug-regs' 
   command.  */

/* Use this function to reset the i386-nat.c debug register state.  */
extern void init_debug_registers();
extern struct debug_reg_state * debug_reg_state();
extern int insert_watchpoint (CORE_ADDR addr, int len, int type, pid_t pid);
extern int remove_watchpoint (CORE_ADDR addr, int len, int type, pid_t pid);
extern unsigned long get_watching_address (pid_t pid);
extern unsigned long get_watching_status (pid_t pid, unsigned long * value);
extern void resume_process (pid_t pid);
extern void pass_signals(pid_t pid, int signal);
extern unsigned long get_eip_addr(pid_t pid);
#endif
