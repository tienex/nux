/** @file
  x86 HAL Internal Definitions

  Internal definitions and declarations for the x86 HAL implementation.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __hal_internal_h__
#define __hal_internal_h__

#include <hal/config.h>

#define MAXCPUS	        HAL_MAXCPUS

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_SCE (1LL << 0)
#define _MSR_IA32_EFER_NXE (1LL << 11)

#define MSR_IA32_FS_BASE 0xc0000100
#define MSR_IA32_GS_BASE 0xc0000101
#define MSR_IA32_KERNEL_GS_BASE 0xc0000102

#define MSR_IA32_STAR 0xc0000081
#define MSR_IA32_LSTAR 0xc0000082
#define MSR_IA32_FMASK 0xc0000084

#define PTE_P       1
#define PTE_W       2
#define PTE_U       4
#define PTE_A       0x20
#define PTE_D       0x40
#define PTE_PS      0x80
#define PTE_G       0x100
#define PTE_AVAIL   0xe00
#define PTE_NX      0x8000000000000000LL

#define PTE_AVAIL0 (1 << 9)
#define PTE_AVAIL1 (2 << 9)
#define PTE_AVAIL2 (4 << 9)

#define l1epfn(_l1e) (((_l1e) &   0x7ffffffffffff000ULL) >> PAGE_SHIFT)
#define l1eflags(_l1e) ((_l1e) & 0x8000000000000fffULL)

#ifndef _ASSEMBLER

#include <nux/nux.h>

#define haldebug(...) debug(__VA_ARGS__)
#define hallog(...) info(__VA_ARGS__)
#define halwarn(...) warn(__VA_ARGS__)
#define halfatal(...) fatal(__VA_ARGS__)

//
// NT-Style Global Variables
//

extern INT32 gNuxInitialized;

//
// NT-Style Function Declarations
//

VOID X86Initialize (VOID);
VOID Amd64Initialize (VOID);
VOID Pae32Initialize (VOID);
VOID Pae32InitializeAp (VOID);
VOID Pae64Initialize (VOID);
VOID Pae64InitializeAp (VOID);
VOID PmapInitialize (VOID);
VOID I386InitializeDone (VOID);
VOID Amd64InitializeDone (VOID);

INT32 InB (IN INT32 Port);
INT32 InW (IN UINT32 Port);
INT32 InL (IN UINT32 Port);
VOID OutB (IN INT32 Port, IN INT32 Val);
VOID OutW (IN UINT32 Port, IN INT32 Val);
VOID OutL (IN UINT32 Port, IN INT32 Val);

typedef UINT64 PTE;
typedef UINTN PTEP;

PTE GetPte (IN PTEP Ptep);
PTE SetPte (IN PTEP Ptep, IN PTE Pte);
hal_l1p_t KmapGetL1p (IN UINTN Va, IN INT32 Alloc);
hal_l1p_t UmapGetL1p (IN struct hal_umap *pUmap, IN UINTN Va, IN INT32 Alloc);
USER_ADDRESS PtUmapNext (
  IN struct hal_umap *pUmap,
  IN USER_ADDRESS Uaddr,
  OUT hal_l1p_t *pL1p OPTIONAL,
  OUT hal_l1e_t *pL1e OPTIONAL
  );
VOID PtUmapFree (IN struct hal_umap *pUmap);
VOID PtUmapDebugWalk (IN struct hal_umap *pUmap OPTIONAL, IN UINTN Va);
UINTN PtUmapMinAddr (VOID);
UINTN PtUmapMaxAddr (VOID);

VOID TlbFlushGlobal (VOID);
VOID TlbFlushLocal (VOID);

VOID SerialInitialize (VOID);
VOID SerialPutChar (IN INT32 Ch);

INT32 VgaPutChar (IN INT32 Ch);

UINT64 ReadMsr (IN UINT32 Ecx);
VOID WriteMsr (IN UINT32 Ecx, IN UINT64 Val);

UINTN ReadCr4 (VOID);
VOID WriteCr4 (IN UINTN r);
UINTN ReadCr3 (VOID);
VOID WriteCr3 (IN UINTN r);

UINTN FrameBp (IN struct hal_frame *pFrame);
UINTN FrameCr2 (IN struct hal_frame *pFrame);

//
// Legacy Function Declarations (for backward compatibility)
//

extern INT32 nux_initialized;

VOID x86_init (VOID);
VOID amd64_init (VOID);
VOID pae32_init (VOID);
VOID pae32_init_ap (VOID);
VOID pae64_init (VOID);
VOID pae64_init_ap (VOID);
VOID pmap_init (VOID);
VOID i386_init_done (VOID);
VOID amd64_init_done (VOID);

INT32 inb (INT32 port);
VOID outb (INT32 port, INT32 val);

typedef UINT64 pte_t;
typedef UINTN ptep_t;

pte_t get_pte (ptep_t ptep);
pte_t set_pte (ptep_t ptep, pte_t pte);
hal_l1p_t kmap_get_l1p (unsigned INTN va, INT32 alloc);
hal_l1p_t umap_get_l1p (struct hal_umap *umap, unsigned INTN va, INT32 alloc);
USER_ADDRESS pt_umap_next (struct hal_umap *umap, USER_ADDRESS uaddr, hal_l1p_t * l1p_out,
		   hal_l1e_t * l1e_out);
VOID pt_umap_free (struct hal_umap *umap);
VOID pt_umap_debugwalk (struct hal_umap *umap, unsigned INTN va);
UINTN pt_umap_minaddr (VOID);
UINTN pt_umap_maxaddr (VOID);

VOID tlbflush_global (VOID);

VOID serial_init (VOID);
VOID serial_putchar (INT32 c);

INT32 vga_putchar (INT32 c);

UINT64 rdmsr (UINT32 ecx);
VOID wrmsr (UINT32 ecx, UINT64 val);

unsigned long read_cr4 (VOID);
VOID write_cr4 (unsigned INTN r);
unsigned long read_cr3 (VOID);
VOID write_cr3 (unsigned INTN r);

unsigned long frame_bp(struct hal_frame *f);
unsigned long frame_cr2(struct hal_frame *f);

#endif

#endif
