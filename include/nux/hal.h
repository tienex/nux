/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _HAL_H
#define _HAL_H

#include <nux/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

/* HAL module specific definitions. */
#include <nux/hal_config.h>


/*
  HAL Abstracted CPU Instructions. 
 */

/*
  I/O ops for architectures with I/O Space. 
 */
unsigned long hal_cpu_in (uint8_t size, uint32_t port);
void hal_cpu_out (uint8_t size, uint32_t port, unsigned long val);

/*
  Relax CPU (spin-wait). 
 */
void hal_cpu_relax (void);

/*
  Trap and generate an exception. 
 */
void hal_cpu_trap (void);

/*
  Set CPU in idle mode 
 */
void __dead hal_cpu_idle (void);

/*
  Halt the cpu. 
 */
void __dead hal_cpu_halt (void);

/*
  Flush CPU's TLBs 
 */
typedef enum
{
  HAL_TLBOP_NONE = 0,		/* No TLB operation.  */
  HAL_TLBOP_FLUSH = (1 << 0),	/* Normal TLB flush.  */
  HAL_TLBOP_FLUSHALL = (1 << 1)	/* Global TLB flush.  */
}
hal_tlbop_t;

void hal_cpu_tlbop (hal_tlbop_t op);

/*
  Set current CPU's local data pointer. 
 */
void hal_cpu_setdata (void *data);

/*
  Return current CPU's local data pointer. 
 */
void *hal_cpu_getdata (void);




/*
  HAL CPU Interrupt Vector description. 
 */

/*
  Vector for first IRQ.
  
  This is used by the platform to program the interrupt controller.
  
  The platform interrupt controller will be programmed to send IRQ 'NUM'
  at CPU vector 'hal_vect_irqbase() + NUM'. 
 */
unsigned hal_vect_irqbase (void);

/*
  First vector available for IPIs.
  
  Vectors starting from this up until either hal_vect_irqbase() (if
  greater) or hal_vect_max() are available to for custom IPIs. 
 */
unsigned hal_vect_ipibase (void);

/*
  Number of vectors (IRQ + IPI) available. 
 */
unsigned hal_vect_max (void);


/*
  HAL Physical Memory Description.
 */

/*
  Maximum Physical Page Number.  
 */
unsigned long hal_physmem_maxpfn (void);

/*
  S-Tree allocation bitmap.

  The HAL is responsible for building at boot
  time a searchable bitmap (see stree.h) that
  contains an up-to date map of free pages.
*/
void *hal_physmem_stree (unsigned *order);


/*
  HAL Virtual Memory Description.
 */

/*
  User Area description. 
 */
vaddr_t hal_virtmem_userbase (void);
const size_t hal_virtmem_usersize (void);

/*
  Direct Memory Map description. 
 */
vaddr_t hal_virtmem_dmapbase (void);
const size_t hal_virtmem_dmapsize (void);

/*
  PFN Cache Area.

  NB: no, this is not VMS.
*/
vaddr_t hal_virtmem_pfn$base (void);
const size_t hal_virtmem_pfn$size (void);

/*
  Kernel Virtual Area Map Description. 
 */
vaddr_t hal_virtmem_kvabase (void);
const size_t hal_virtmem_kvasize (void);

/*
  Kernel Memory Area Description.
*/
vaddr_t hal_virtmem_kmembase (void);
const size_t hal_virtmem_kmemsize (void);

/*
  HAL Virtual Memory Mapping.
 */

/*
  Return current pmap.

  If returns NULL, we're still running the boot-time PMAP.
 */
struct hal_pmap *hal_pmap_current (void);

/*
  Initialise new pmap. 
 */
bool hal_pmap_create (struct hal_pmap *pmap);

/*
  Finalise new pmap. 
 */
void hal_pmap_destroy (struct hal_pmap *pmap);

/*
  Initialise HAL pmap subsystem. 
 */
void hal_pmap_init (void);

/*
  Set current pmap 
 */
void hal_pmap_setcur (struct hal_pmap *pmap);

/*
  Do a page walk (populating pagetables if ALLOC is true).

  If an l1p exists, save it into L1P and return true.

  If no l1p is found, set L1P to L1P_INVALID and return false.

  Note that it might still return false with ALLOC=true if out of memory.
*/
bool hal_pmap_getl1p (struct hal_pmap *pmap, unsigned long va, bool alloc,
		      hal_l1p_t *l1p);

/*
  Get L1E pointed by L1P.
*/
hal_l1e_t hal_pmap_getl1e (struct hal_pmap *pmap, hal_l1p_t l1p);

/*
  Install an L1E in the pmap 

  If PMAP is NULL, set in current pmap.
 */
hal_l1e_t hal_pmap_setl1e (struct hal_pmap *pmap, hal_l1p_t l1p,
			   hal_l1e_t new);

#define HAL_PTE_P      (1 << 0)
#define HAL_PTE_W      (1 << 1)
#define HAL_PTE_X      (1 << 2)
#define HAL_PTE_U      (1 << 3)
#define HAL_PTE_GLOBAL (1 << 4)
#define HAL_PTE_AVL0   (1 << 5)
#define HAL_PTE_AVL1   (1 << 6)
#define HAL_PTE_AVL2   (1 << 7)

/*
  Create a l1e value 
 */
hal_l1e_t hal_pmap_boxl1e (unsigned long pfn, unsigned flags);

/*
  Decompose a l1e value 
 */
void hal_pmap_unboxl1e (hal_l1e_t l1e, unsigned long *pfn, unsigned *prot);

/*
  TLB flush operation for a PT change. 
 */
hal_tlbop_t hal_pmap_tlbop (hal_l1e_t old, hal_l1e_t new);


/*
  HAL PCPU: Physical CPU bringup and setup.
 */

/*
  Initialisation of HAL CPU <PCPUID>.
  
  Provide the requested data structure to the HAL for its internal use. 
 */
void hal_pcpu_init (unsigned pcpuid, struct hal_cpu *haldata);

/*
  Load HAL-specific status for the current CPU..
  
  This will be called, once for each CPU, at initialisation time. 
 */
void hal_pcpu_enter (unsigned pcpuid);

/*
  Prepare machine for booting cpu PCPU. Returns address where CPU
  bootstrap code starts.. 
 */
uint64_t hal_pcpu_prepare (unsigned pcpu);


/*
  Hardware Abstraction Layer: Interrupt Frame.

  The HAL communicates information about the status of the CPU when
  an interrupt/exception occurred with the struct hal_frame, which
  contains all the CPU register status.

  This structure contains all the information required to restore the
  CPU on interrupt return, and the HAL provides some abstracted
  modifiers.
*/

/*
  Initialise a HAL frame.

  Instruction Pointer and Stack Pointer are left cleared.
*/
void hal_frame_init (struct hal_frame *f);

/*
  Check that the HAL frame originated in user space.

  NMI, exception and idle can generate frame from kernel mode.
*/
bool hal_frame_isuser (struct hal_frame *f);

/* Set frame instruction pointer */
void hal_frame_setip (struct hal_frame *f, unsigned long ip);

/* Get frame instruction pointer */
unsigned long hal_frame_getip (struct hal_frame *f);

/* Set frame stack pointer */
void hal_frame_setsp (struct hal_frame *f, unsigned long sp);

/* Get frame stack pointer */
unsigned long hal_frame_getsp (struct hal_frame *f);

/*
  Set frame argument registers

  We define here three HAL-dependent registers, a0, a1, a2, that can be used
  as argument on entry.
*/
void hal_frame_seta0 (struct hal_frame *f, unsigned long a0);
void hal_frame_seta1 (struct hal_frame *f, unsigned long a1);
void hal_frame_seta2 (struct hal_frame *f, unsigned long a2);


/*
  Set frame return value register.

  In case of a syscall, this will allow user code to see a return value.
  The return value register might be one of the arguments.
*/
void hal_frame_setret (struct hal_frame *f, unsigned long r);

/* Print frame information to log. */
void hal_frame_print (struct hal_frame *);

/*
  Send a signal to the userspace.

  This function updates the frame's IP and expands the stack. Writes a
  new stack frame that will allow a function to return normally to the
  interrupted code.

  The HAL will simply write to user address space. Users of this calls
  will have to guarantee that the user address is mapped. HAL needs to
  check that the address is effectively userspace and not kernel.
*/
bool hal_frame_signal (struct hal_frame *f, unsigned long ip, unsigned long arg);


/*
  Hardware Abstraction Layer: System Entries.
 
  The following are callbacks to the main program. These calls are
  the input to the kernel.
 */

#define HAL_PF_REASON_PROT 0;
#define HAL_PF_REASON_NOTP 1;
#define HAL_PF_INFO_WRITE (1 << 4);
#define HAL_PF_INFO_USER  (1 << 5);

typedef unsigned hal_pfinfo_t;

/*
  Page Fault. (can be in kernel) 
 */
struct hal_frame *hal_entry_pf (struct hal_frame *, unsigned long,
				hal_pfinfo_t);

/*
  Exception. (can be in kernel) 
 */
struct hal_frame *hal_entry_xcpt (struct hal_frame *, unsigned);

/*
  NMI: interrupt to CPU. (can be in kernel) 
 */
struct hal_frame *hal_entry_nmi (struct hal_frame *f);

/*
  Interrupts: IPIs and IRQs
 */
struct hal_frame *hal_entry_vect (struct hal_frame *f, unsigned irq);

/*
  System Call 
 */
struct hal_frame *hal_entry_syscall (struct hal_frame *,
				     unsigned long, unsigned long,
				     unsigned long, unsigned long,
				     unsigned long, unsigned long);

/*
  Secondary CPUs start up. 
 */
void hal_main_ap (void);


/*
  Miscellaneous. 
 */

/*
  Boot-time put character to screen. 
 */
int hal_putchar (int c);

/*
  Platform Information acquired from bootloader.

  This should be a generic as possible, and should be extended to
  support fields for every platform.
*/
struct hal_pltinfo_desc {
  uint64_t acpi_rsdp;
};

const struct hal_pltinfo_desc *
hal_pltinfo (void);

/*
  Stop all CPUs and panic. Of course you'll never need to call this!
*/
__dead void hal_panic (void);

#endif /* _HAL_H */


