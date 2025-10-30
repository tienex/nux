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

#include <hal/hal.h>
#include <platform/platform.h>

#include <hal/arch/i386/i386.h>
#include <hal/internal.h>

PHYSICAL_ADDRESS gPcpuPstart;
VIRTUAL_ADDRESS gPcpuHalData[MAXCPUS];

/*
  CPU kernel stack allocation:

   CPUs at boot allocate a stack by incrementing (with a spinlock)
   'gPcpuKStackNo' and using the page pointed at 'gPcpuKStack'.

   Importantly, gPcpuKStack[pcpu_id] doesn't mean is the stack of PCPU ID.
*/
UINT64 gPcpuKStackNo = 0;
UINT64 gPcpuKStackCnt = 0;
UINT64 gPcpuKStack[MAXCPUS];

static int gBspEnterCalled = 0;
static UINT32 gBspPcpuId;

static VIRTUAL_ADDRESS gSmpOldVa;
static hal_l1e_t gSmpOldL1e;

/**
  Get FS segment selector for current CPU.

  @return FS segment selector value.
**/
UINT16
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
  PFN Pfn;
  VOID *Start, *Ptr;
  hal_l1p_t L1p;
  PHYSICAL_ADDRESS PStart;
  VOLATILE UINT16 *Reset;
  extern CHAR8 *_ap_start, *_ap_end;

  /* Allocate PCPU bootstrap code. Use KVA. *//* TODO: USE KVA? Not needed, not a long term mapping. */
  Pfn = pfn_alloc (1);
  assert (Pfn != PFN_INVALID);
  /* This is tricky. The hope is that is low enough to be addressed by 16 bit. */
  assert (Pfn < (1 << 8) && "Can't allocate Memory below 1MB!");

  /* Map and prepare the bootstrap code page. */
  Start = pfn_get (Pfn);
  UINTN ApBootSz = (UINTN) ((VOID *) &_ap_end - (VOID *) &_ap_start);
  assert (ApBootSz <= PAGE_SIZE);
  memcpy (Start, &_ap_start, ApBootSz);
  PStart = (PHYSICAL_ADDRESS) Pfn << PAGE_SHIFT;

  /*
     The following is trampoline dependent code, and configures the
     trampoline to use the page just selected as bootstrap page.
   */
  extern UINT8 _ap_gdtreg, _ap_ljmp, _ap_cr3;
  extern UINT32 _bsp_cr3;

  /* Copy BSP CR3 into AP */
  Ptr = Start + ((VOID *) &_ap_cr3 - (VOID *) &_ap_start);
  *(UINT32 *) Ptr = _bsp_cr3;

  /* Setup temporary GDT register. */
  Ptr = Start + ((VOID *) &_ap_gdtreg - (VOID *) &_ap_start);
  *(UINT32 *) (Ptr + 2) += (UINT32) PStart;

  /* Setup trampoline 1 */
  Ptr = Start + ((VOID *) &_ap_ljmp - (VOID *) &_ap_start);
  *(UINT32 *) Ptr += (UINT32) PStart;

  pfn_put (Pfn, Start);

  /* Set reset vector */
  Reset = kva_physmap (0x467, 2, HAL_PTE_P | HAL_PTE_W | HAL_PTE_X);
  *Reset = PStart & 0xf;
  *(Reset + 1) = PStart >> 4;
  kva_unmap ((VOID *) Reset, 2);

  /* PStart is in user address space: use kmap_ instead of hal_kmap */
  L1p = umap_get_l1p (NULL, PStart, TRUE);
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
  @param[in] HalData Pointer to HAL CPU data structure.
**/
VOID
HalPcpuAdd (
  IN unsigned          PcpuId,
  IN struct hal_cpu   *HalData
  )
{
  PFN Pfn;
  VOID *Va;
  void _set_tss (unsigned, VOID *);
  void _set_fs (unsigned, VOID *);

  assert (PcpuId < MAXCPUS);

  if (PcpuId == gBspPcpuId)
    {
      /* Adding the BSP PCPU: Initialize TSS */
      extern UINT8 _bsp_stacktop;
      HalData->tss.ss0 = KDS;
      HalData->tss.esp0 = (UINTN) & _bsp_stacktop;
      HalData->tss.iomap = 108;
    }
  else
    {
      /* Adding secondary CPU: Allocate one PCPU kernel stack. */
      Pfn = pfn_alloc (1);
      assert (Pfn != PFN_INVALID);
      Va = kva_map (Pfn, HAL_PTE_W | HAL_PTE_P);
      assert (Va != NULL);
      gPcpuKStack[gPcpuKStackNo++] = (UINT64) (UINTN) Va + PAGE_SIZE;
    }
  _set_tss (PcpuId, &HalData->tss);
  _set_fs (PcpuId, &HalData->data);

  gPcpuHalData[PcpuId] = (VIRTUAL_ADDRESS) (UINTN) HalData;
}

/**
  Get physical start address for per-CPU bootstrap.

  @param[in] Pcpu  Per-CPU ID.

  @return Physical address of bootstrap code, or PADDR_INVALID.
**/
UINT64
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
  UINT16 Tss = (5 + 4 * PcpuId) << 3;
  UINT16 Fs = (5 + 4 * PcpuId + 1) << 3;

  assert (PcpuId < MAXCPUS);

  asm volatile ("ltr %%ax"::"a" (Tss));
  asm volatile ("mov %%ax, %%fs"::"a" (Fs));

  gBspEnterCalled = 1;
}

/**
  Set per-CPU data pointer.

  @param[in] Data  Pointer to per-CPU data.
**/
VOID
HalCpuSetData (
  IN VOID  *Data
  )
{
  asm volatile ("movl %0, %%fs:0\n"::"r" (Data));
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
  VOID *Data;

  asm volatile ("movl %%fs:0, %0\n":"=r" (Data));
  return Data;
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
  IN UINTN  Esp
  )
{
  unsigned Pcpu = plt_pcpu_id ();
  struct hal_cpu *HalData = (struct hal_cpu *) (UINTN) gPcpuHalData[Pcpu];

  HalData->tss.ss0 = KDS;
  HalData->tss.esp0 = Esp;
  HalData->tss.iomap = 108;

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
  L1p = umap_get_l1p (NULL, gSmpOldVa, FALSE);
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
UINT16 _i386_fs (VOID) {
  return I386GetFsSelector ();
}

/** @deprecated Use HalPcpuInit instead **/
VOID hal_pcpu_init (VOID) {
  HalPcpuInit ();
}

/** @deprecated Use HalPcpuAdd instead **/
VOID hal_pcpu_add (UINT32 pcpuid, struct hal_cpu *haldata) {
  HalPcpuAdd (pcpuid, haldata);
}

/** @deprecated Use HalPcpuStartAddr instead **/
UINT64 hal_pcpu_startaddr (UINT32 pcpu) {
  return HalPcpuStartAddr (pcpu);
}

/** @deprecated Use HalPcpuEnter instead **/
VOID hal_pcpu_enter (UINT32 pcpuid) {
  HalPcpuEnter (pcpuid);
}

/** @deprecated Use HalCpuSetData instead **/
VOID hal_cpu_setdata (VOID *data) {
  HalCpuSetData (data);
}

/** @deprecated Use HalCpuGetData instead **/
VOID * hal_cpu_getdata (VOID) {
  return HalCpuGetData ();
}

/** @deprecated Use HalVectMax instead **/
UINT32 hal_vect_max (VOID) {
  return HalVectMax ();
}

/** @deprecated Use I386InitializeAp instead **/
VOID i386_init_ap (UINTN esp) {
  I386InitializeAp (esp);
}

/** @deprecated Use RemoveBootMappings instead **/
static VOID remove_bootmappings (VOID) {
  RemoveBootMappings ();
}

/** @deprecated Use I386InitializeDone instead **/
VOID i386_init_done (VOID) {
  I386InitializeDone ();
}

// Legacy global variable aliases
PHYSICAL_ADDRESS pcpu_pstart = 0;
VIRTUAL_ADDRESS pcpu_haldata[MAXCPUS] = {0};
UINT64 pcpu_kstackno = 0;
UINT64 pcpu_kstackcnt = 0;
UINT64 pcpu_kstack[MAXCPUS] = {0};
