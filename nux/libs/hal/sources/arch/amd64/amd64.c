/** @file
  AMD64 Architecture-Specific Initialization

  Provides AMD64-specific CPU initialization, per-CPU data management,
  GDT/TSS setup, SMP bootstrap code configuration, and system call MSR setup.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <hal.h>
#include <platform.h>
#include "hal/arch/amd64/amd64.h"
#include "../internal.h"

extern uint64_t _gdt[];
extern int _physmap_start;
extern int _physmap_end;

paddr_t gPcpuPstart;
vaddr_t gPcpuHalData[MAXCPUS];

static unsigned gBspPcpuId;

static vaddr_t gSmpOldVa;
static hal_l1e_t gSmpOldL1e;

/*
  CPU kernel stack allocation:

   CPUs at boot allocate a stack by incrementing (with a spinlock)
   'gPcpuKStackNo' and using the page pointed at 'gPcpuKStack'.

   Importantly, gPcpuKStack[pcpu_id] doesn't mean is the stack of PCPU ID.
*/
uint64_t gPcpuKStackNo = 0;
uint64_t gPcpuKStackCnt = 0;
uint64_t gPcpuKStack[MAXCPUS];

/**
  Set Task State Segment (TSS) in Global Descriptor Table (GDT).

  @param[in] PcpuId  Per-CPU ID.
  @param[in] pTss    Pointer to TSS structure.
**/
VOID
GdtSetTss (
  IN unsigned           PcpuId,
  IN struct amd64_tss  *pTss
  )
{
  uintptr_t Ptr = (uintptr_t) pTss;
  uint16_t Lo16 = (uint16_t) Ptr;
  uint8_t Ml8 = (uint8_t) (Ptr >> 16);
  uint8_t Mh8 = (uint8_t) (Ptr >> 24);
  uint32_t Hi32 = (uint32_t) (Ptr >> 32);
  uint16_t Limit = sizeof (*pTss);
  uint32_t *pPtr32 = (uint32_t *) (_gdt + TSS_GDTIDX (PcpuId));

  pPtr32[0] = Limit | ((uint32_t) Lo16 << 16);
  pPtr32[1] = Ml8 | (0x0089 << 8) | ((uint32_t) Mh8 << 24);
  pPtr32[2] = Hi32;
  pPtr32[3] = 0;
}

/**
  Set kernel GS base MSR.

  @param[in] GsBase  GS base address.
**/
VOID
SetKernelGsBase (
  IN unsigned long  GsBase
  )
{
  wrmsr (MSR_IA32_KERNEL_GS_BASE, GsBase);
}

/**
  Set GS base MSR.

  @param[in] GsBase  GS base address.
**/
VOID
SetGsBase (
  IN unsigned long  GsBase
  )
{
  wrmsr (MSR_IA32_GS_BASE, GsBase);
}

/**
  Get GS base MSR value.

  @return Current GS base address.
**/
uint64_t
GetGsBase (
  VOID
  )
{
  return rdmsr (MSR_IA32_GS_BASE);
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
  asm volatile ("movq %0, %%gs:0\n"::"r" (pData));
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

  asm volatile ("movq %%gs:0, %0\n":"=r" (pData));
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
  return VECT_MAX;
}

#define CANARY_SIZE PAGE_SIZE
#define STACK_SIZE (64 * 1024) /* 64kb Stack. */

/**
  Allocate kernel stack page with guard pages.

  @return Virtual address of top of allocated stack.
**/
static uint64_t
AllocStackPage (
  VOID
  )
{
  vaddr_t KAddr;

  /* Leave a canary page at beginning and end of kernel stack. */
  KAddr = kva_alloc (STACK_SIZE + 2 * CANARY_SIZE);
  assert (KAddr != VADDR_INVALID);
  assert (!kmap_ensure_range (KAddr + CANARY_SIZE, STACK_SIZE, HAL_PTE_W | HAL_PTE_P));

  return KAddr + CANARY_SIZE + STACK_SIZE;
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

  assert (PcpuId < MAXCPUS);

  gPcpuHalData[PcpuId] = (vaddr_t) (uintptr_t) pHalData;
  GdtSetTss (PcpuId, &pHalData->tss);

  if (PcpuId == gBspPcpuId)
    {
      /* Adding the BSP PCPU: Initialize TSS */
      extern char _bsp_stacktop, _ist1_stacktop, _ist1_stacktop,
	_ist2_stacktop, _ist3_stacktop;
      pHalData->kstack = (uintptr_t) & _bsp_stacktop;
      pHalData->tss.ist[0] = (uintptr_t) & _ist1_stacktop;
      pHalData->tss.ist[1] = (uintptr_t) & _ist2_stacktop;
      pHalData->tss.ist[2] = (uintptr_t) & _ist3_stacktop;
      pHalData->tss.rsp0 = (uintptr_t) & _bsp_stacktop;
      pHalData->tss.iomap = 108;
    }
  else
    {
      /* Adding secondary CPU: Allocate one PCPU kernel stack. */
      gPcpuKStack[gPcpuKStackNo++] = AllocStackPage ();
    }
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
  void *pVa;
  pfn_t Pfn;
  void *pStart, *pPtr;
  hal_l1p_t L1p;
  paddr_t PStart;
  volatile uint16_t *pReset;
  extern char *_ap_start, *_ap_end;

  /* Allocate PCPU bootstrap code page. */
  Pfn = pfn_alloc (1);
  /* This is tricky. The hope is that is low enough to be addressed by
     16 bit. */
  assert (Pfn < (1 << 8) && "Can't allocate Memory below 1MB!");

  /* Map and prepare the bootstrap code page. */
  pVa = kva_map (Pfn, HAL_PTE_W | HAL_PTE_P);
  assert (pVa != NULL);
  pStart = pVa;
  size_t ApBootSz = (size_t) ((void *) &_ap_end - (void *) &_ap_start);
  assert (ApBootSz <= PAGE_SIZE);
  memcpy (pStart, &_ap_start, ApBootSz);

  PStart = (paddr_t) Pfn << PAGE_SHIFT;

  /*
     The following is trampoline dependent code, and configures the
     trampoline to use the page just selected as bootstrap page.
   */
  extern char _ap_gdtreg, _ap_ljmp1, _ap_ljmp2, _ap_cr3;
  extern uint64_t _bsp_cr3;

  /* Copy BSP CR3 into AP */
  pPtr = pStart + ((void *) &_ap_cr3 - (void *) &_ap_start);
  *(uint64_t *) pPtr = _bsp_cr3;

  /* Setup temporary GDT register. */
  pPtr = pStart + ((void *) &_ap_gdtreg - (void *) &_ap_start);
  *(uint32_t *) (pPtr + 2) += (uint32_t) PStart;

  /* Setup trampoline 1 */
  pPtr = pStart + ((void *) &_ap_ljmp1 - (void *) &_ap_start);
  *(uint32_t *) pPtr += (uint32_t) PStart;

  /* Setup trampoline 2 */
  pPtr = pStart + ((void *) &_ap_ljmp2 - (void *) &_ap_start);
  *(uint32_t *) pPtr += (uint32_t) PStart;

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
  Get physical start address for per-CPU bootstrap.

  @param[in] Pcpu  Per-CPU ID.

  @return Physical address of bootstrap code, or PADDR_INVALID.
**/
paddr_t
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
  assert (PcpuId < MAXCPUS);

  SetGsBase (gPcpuHalData[PcpuId]);
  SetKernelGsBase (gPcpuHalData[PcpuId]);

  asm volatile ("ltr %%ax"::"a" (TSS_GDTIDX (PcpuId) << 3));
}

/**
  Initialize AMD64 architecture (BSP).

  Sets up SYSCALL/SYSRET support via MSRs.
**/
VOID
Amd64Initialize (
  VOID
  )
{
  extern char _syscall_frame_entry;
  wrmsr (MSR_IA32_EFER, rdmsr (MSR_IA32_EFER) | _MSR_IA32_EFER_SCE);
  wrmsr (MSR_IA32_LSTAR, (uintptr_t) & _syscall_frame_entry);
  wrmsr (MSR_IA32_FMASK, 0xfffffffd);
  wrmsr (MSR_IA32_STAR, ((uint64_t) KCS << 32) | ((uint64_t) UCS32 << 48));
}

/**
  Initialize AMD64 architecture (AP).

  Sets up SYSCALL/SYSRET support and allocates per-CPU stacks for AP.

  @param[in] Esp  Stack pointer for this AP.
**/
VOID
Amd64InitializeAp (
  IN uintptr_t  Esp
  )
{
  extern char _syscall_frame_entry;
  unsigned Pcpu = plt_pcpu_id ();
  struct hal_cpu *pHalData = (struct hal_cpu *) (uintptr_t) gPcpuHalData[Pcpu];

  wrmsr (MSR_IA32_EFER, rdmsr (MSR_IA32_EFER) | _MSR_IA32_EFER_SCE);
  wrmsr (MSR_IA32_LSTAR, (uintptr_t) & _syscall_frame_entry);
  wrmsr (MSR_IA32_FMASK, 0xfffffffd);
  wrmsr (MSR_IA32_STAR, ((uint64_t) KCS << 32) | ((uint64_t) UCS32 << 48));

  pHalData->kstack = Esp;
  pHalData->tss.ist[0] = AllocStackPage ();
  pHalData->tss.ist[1] = AllocStackPage ();
  pHalData->tss.ist[2] = AllocStackPage ();
  pHalData->tss.rsp0 = Esp;
  pHalData->tss.iomap = 108;

  pae64_init_ap ();

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

  /*
     Restore the mapping created for boostrapping secondary CPUS.
     Use UMAP, as it is in the user address space.
   */
  L1p = umap_get_l1p (NULL, gSmpOldVa, false);
  assert (L1p != L1P_INVALID);
  hal_l1e_set (L1p, gSmpOldL1e);
}

/**
  Finalize AMD64 initialization.

  Cleans up temporary bootstrap mappings.
**/
VOID
Amd64InitializeDone (
  VOID
  )
{

  RemoveBootMappings ();
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use GdtSetTss instead **/
void gdt_settss (unsigned pcpuid, struct amd64_tss *tss) {
  GdtSetTss (pcpuid, tss);
}

/** @deprecated Use SetKernelGsBase instead **/
void set_kernel_gsbase (unsigned long gsbase) {
  SetKernelGsBase (gsbase);
}

/** @deprecated Use SetGsBase instead **/
void set_gsbase (unsigned long gsbase) {
  SetGsBase (gsbase);
}

/** @deprecated Use GetGsBase instead **/
uint64_t get_gsbase (void) {
  return GetGsBase ();
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

/** @deprecated Use AllocStackPage instead **/
static uint64_t alloc_stackpage (void) {
  return AllocStackPage ();
}

/** @deprecated Use HalPcpuAdd instead **/
void hal_pcpu_add (unsigned pcpuid, struct hal_cpu *haldata) {
  HalPcpuAdd (pcpuid, haldata);
}

/** @deprecated Use HalPcpuInit instead **/
void hal_pcpu_init (void) {
  HalPcpuInit ();
}

/** @deprecated Use HalPcpuStartAddr instead **/
paddr_t hal_pcpu_startaddr (unsigned pcpu) {
  return HalPcpuStartAddr (pcpu);
}

/** @deprecated Use HalPcpuEnter instead **/
void hal_pcpu_enter (unsigned pcpuid) {
  HalPcpuEnter (pcpuid);
}

/** @deprecated Use Amd64Initialize instead **/
void amd64_init (void) {
  Amd64Initialize ();
}

/** @deprecated Use Amd64InitializeAp instead **/
void amd64_init_ap (uintptr_t esp) {
  Amd64InitializeAp (esp);
}

/** @deprecated Use RemoveBootMappings instead **/
static void remove_bootmappings (void) {
  RemoveBootMappings ();
}

/** @deprecated Use Amd64InitializeDone instead **/
void amd64_init_done (void) {
  Amd64InitializeDone ();
}

// Legacy global variable aliases
paddr_t pcpu_pstart = 0;
vaddr_t pcpu_haldata[MAXCPUS] = {0};
uint64_t pcpu_kstackno = 0;
uint64_t pcpu_kstackcnt = 0;
uint64_t pcpu_kstack[MAXCPUS] = {0};
