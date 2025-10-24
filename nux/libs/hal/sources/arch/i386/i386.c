/** @file
  i386 Architecture-Specific Initialization

  Provides i386-specific CPU initialization, per-CPU data management,
  TSS/FS segment setup, SMP bootstrap code configuration.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <cdefs.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <nux/hal.h>
#include <nux/plt.h>

#include "hal/arch/i386/i386.h"
#include "../internal.h"

paddr_t gPcpuPstart;
vaddr_t gPcpuHalData[MAXCPUS];

/*
  CPU kernel stack allocation:

   CPUs at boot allocate a stack by incrementing (with a spinlock)
   'gPcpuKStackNo' and using the page pointed at 'gPcpuKStack'.

   Importantly, gPcpuKStack[pcpu_id] doesn't mean is the stack of PCPU ID.
*/
uint64_t gPcpuKStackNo = 0;
uint64_t gPcpuKStackCnt = 0;
uint64_t gPcpuKStack[MAXCPUS];

static int gBspEnterCalled = 0;
static unsigned gBspPcpuId;

static vaddr_t gSmpOldVa;
static hal_l1e_t gSmpOldL1e;

/**
  Get FS segment selector for current CPU.

  @return FS segment selector value.
**/
uint16_t
I386GetFsSelector (
  VOID
  )
{
  if (gBspEnterCalled)
    return (5 + 4 * plt_pcpu_id () + 1) << 3;
  else
    return 0;
}

/**
  Initialize per-CPU bootstrap code.

  Sets up SMP bootstrap code page at low memory and configures
  reset vector for AP startup.
**/
VOID
HalPcpuInit (
  VOID
  )
{
  pfn_t Pfn;
  void *pStart, *pPtr;
  hal_l1p_t L1p;
  paddr_t PStart;
  volatile uint16_t *pReset;
  extern char *_ap_start, *_ap_end;

  /* Allocate PCPU bootstrap code. Use KVA. *//* TODO: USE KVA? Not needed, not a long term mapping. */
  Pfn = pfn_alloc (1);
  assert (Pfn != PFN_INVALID);
  /* This is tricky. The hope is that is low enough to be addressed by 16 bit. */
  assert (Pfn < (1 << 8) && "Can't allocate Memory below 1MB!");

  /* Map and prepare the bootstrap code page. */
  pStart = pfn_get (Pfn);
  size_t ApBootSz = (size_t) ((void *) &_ap_end - (void *) &_ap_start);
  assert (ApBootSz <= PAGE_SIZE);
  memcpy (pStart, &_ap_start, ApBootSz);
  PStart = (paddr_t) Pfn << PAGE_SHIFT;

  /*
     The following is trampoline dependent code, and configures the
     trampoline to use the page just selected as bootstrap page.
   */
  extern char _ap_gdtreg, _ap_ljmp, _ap_cr3;
  extern uint32_t _bsp_cr3;

  /* Copy BSP CR3 into AP */
  pPtr = pStart + ((void *) &_ap_cr3 - (void *) &_ap_start);
  *(uint32_t *) pPtr = _bsp_cr3;

  /* Setup temporary GDT register. */
  pPtr = pStart + ((void *) &_ap_gdtreg - (void *) &_ap_start);
  *(uint32_t *) (pPtr + 2) += (uint32_t) PStart;

  /* Setup trampoline 1 */
  pPtr = pStart + ((void *) &_ap_ljmp - (void *) &_ap_start);
  *(uint32_t *) pPtr += (uint32_t) PStart;

  pfn_put (Pfn, pStart);

  /* Set reset vector */
  pReset = kva_physmap (0x467, 2, HAL_PTE_P | HAL_PTE_W | HAL_PTE_X);
  *pReset = PStart & 0xf;
  *(pReset + 1) = PStart >> 4;
  kva_unmap ((void *) pReset, 2);

  /* PStart is in user address space: use kmap_ instead of hal_kmap */
  L1p = umap_get_l1p (NULL, PStart, true);
  assert (L1p != L1P_INVALID);
  /* Save the l1e we're abou to overwrite. We'll restore it after init is done. */
  gSmpOldL1e = hal_l1e_get (L1p);
  gSmpOldVa = PStart;
  hal_l1e_set (L1p, (PStart & ~PAGE_MASK) | PTE_P | PTE_W);
  gPcpuPstart = PStart;

  gBspPcpuId = plt_pcpu_id ();
}

/**
  Add per-CPU data structure.

  @param[in] PcpuId   Per-CPU ID.
  @param[in] pHalData Pointer to HAL CPU data structure.
**/
VOID
HalPcpuAdd (
  IN unsigned          PcpuId,
  IN struct hal_cpu   *pHalData
  )
{
  pfn_t Pfn;
  void *pVa;
  void _set_tss (unsigned, void *);
  void _set_fs (unsigned, void *);

  assert (PcpuId < MAXCPUS);

  if (PcpuId == gBspPcpuId)
    {
      /* Adding the BSP PCPU: Initialize TSS */
      extern char _bsp_stacktop;
      pHalData->tss.ss0 = KDS;
      pHalData->tss.esp0 = (uintptr_t) & _bsp_stacktop;
      pHalData->tss.iomap = 108;
    }
  else
    {
      /* Adding secondary CPU: Allocate one PCPU kernel stack. */
      Pfn = pfn_alloc (1);
      assert (Pfn != PFN_INVALID);
      pVa = kva_map (Pfn, HAL_PTE_W | HAL_PTE_P);
      assert (pVa != NULL);
      gPcpuKStack[gPcpuKStackNo++] = (uint64_t) (uintptr_t) pVa + PAGE_SIZE;
    }
  _set_tss (PcpuId, &pHalData->tss);
  _set_fs (PcpuId, &pHalData->data);

  gPcpuHalData[PcpuId] = (vaddr_t) (uintptr_t) pHalData;
}

/**
  Get physical start address for per-CPU bootstrap.

  @param[in] Pcpu  Per-CPU ID.

  @return Physical address of bootstrap code, or PADDR_INVALID.
**/
uint64_t
HalPcpuStartAddr (
  IN unsigned  Pcpu
  )
{
  if (Pcpu >= MAXCPUS)
    return PADDR_INVALID;

  return gPcpuPstart;
}

/**
  Enter per-CPU context.

  @param[in] PcpuId  Per-CPU ID to enter.
**/
VOID
HalPcpuEnter (
  IN unsigned  PcpuId
  )
{
  uint16_t Tss = (5 + 4 * PcpuId) << 3;
  uint16_t Fs = (5 + 4 * PcpuId + 1) << 3;

  assert (PcpuId < MAXCPUS);

  asm volatile ("ltr %%ax"::"a" (Tss));
  asm volatile ("mov %%ax, %%fs"::"a" (Fs));

  gBspEnterCalled = 1;
}

/**
  Set per-CPU data pointer.

  @param[in] pData  Pointer to per-CPU data.
**/
VOID
HalCpuSetData (
  IN VOID  *pData
  )
{
  asm volatile ("movl %0, %%fs:0\n"::"r" (pData));
}

/**
  Get per-CPU data pointer.

  @return Pointer to per-CPU data.
**/
VOID *
HalCpuGetData (
  VOID
  )
{
  void *pData;

  asm volatile ("movl %%fs:0, %0\n":"=r" (pData));
  return pData;
}

/**
  Get maximum interrupt vector number.

  @return Maximum vector number supported.
**/
unsigned
HalVectMax (
  VOID
  )
{
  return 255;
}

/**
  Initialize i386 architecture (AP).

  Sets up TSS for Application Processor.

  @param[in] Esp  Stack pointer for this AP.
**/
VOID
I386InitializeAp (
  IN uintptr_t  Esp
  )
{
  unsigned Pcpu = plt_pcpu_id ();
  struct hal_cpu *pHalData = (struct hal_cpu *) (uintptr_t) gPcpuHalData[Pcpu];

  pHalData->tss.ss0 = KDS;
  pHalData->tss.esp0 = Esp;
  pHalData->tss.iomap = 108;

  pae32_init_ap ();

  hal_main_ap ();
}

/**
  Remove bootstrap mappings.

  Restores the page table entry that was overwritten during SMP init.
**/
static VOID
RemoveBootMappings (
  VOID
  )
{
  hal_l1p_t L1p;

  /* Restore the mapping created for boostrapping secondary CPUS. */
  L1p = umap_get_l1p (NULL, gSmpOldVa, false);
  assert (L1p != L1P_INVALID);
  hal_l1e_set (L1p, gSmpOldL1e);
}

/**
  Finalize i386 initialization.

  Cleans up temporary bootstrap mappings.
**/
VOID
I386InitializeDone (
  VOID
  )
{

  RemoveBootMappings ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use I386GetFsSelector instead **/
uint16_t _i386_fs (void) {
  return I386GetFsSelector ();
}

/** @deprecated Use HalPcpuInit instead **/
void hal_pcpu_init (void) {
  HalPcpuInit ();
}

/** @deprecated Use HalPcpuAdd instead **/
void hal_pcpu_add (unsigned pcpuid, struct hal_cpu *haldata) {
  HalPcpuAdd (pcpuid, haldata);
}

/** @deprecated Use HalPcpuStartAddr instead **/
uint64_t hal_pcpu_startaddr (unsigned pcpu) {
  return HalPcpuStartAddr (pcpu);
}

/** @deprecated Use HalPcpuEnter instead **/
void hal_pcpu_enter (unsigned pcpuid) {
  HalPcpuEnter (pcpuid);
}

/** @deprecated Use HalCpuSetData instead **/
void hal_cpu_setdata (void *data) {
  HalCpuSetData (data);
}

/** @deprecated Use HalCpuGetData instead **/
void * hal_cpu_getdata (void) {
  return HalCpuGetData ();
}

/** @deprecated Use HalVectMax instead **/
unsigned hal_vect_max (void) {
  return HalVectMax ();
}

/** @deprecated Use I386InitializeAp instead **/
void i386_init_ap (uintptr_t esp) {
  I386InitializeAp (esp);
}

/** @deprecated Use RemoveBootMappings instead **/
static void remove_bootmappings (void) {
  RemoveBootMappings ();
}

/** @deprecated Use I386InitializeDone instead **/
void i386_init_done (void) {
  I386InitializeDone ();
}

// Legacy global variable aliases
paddr_t pcpu_pstart = 0;
vaddr_t pcpu_haldata[MAXCPUS] = {0};
uint64_t pcpu_kstackno = 0;
uint64_t pcpu_kstackcnt = 0;
uint64_t pcpu_kstack[MAXCPUS] = {0};
