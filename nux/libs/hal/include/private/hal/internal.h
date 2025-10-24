/** @file
  x86 HAL Internal Definitions

  Internal definitions and declarations for the x86 HAL implementation.

  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <config.h>

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

#include <nux.h>

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
uaddr_t PtUmapNext (
  IN struct hal_umap *pUmap,
  IN uaddr_t Uaddr,
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

extern int nux_initialized;

void x86_init (void);
void amd64_init (void);
void pae32_init (void);
void pae32_init_ap (void);
void pae64_init (void);
void pae64_init_ap (void);
void pmap_init (void);
void i386_init_done (void);
void amd64_init_done (void);

int inb (int port);
void outb (int port, int val);

typedef uint64_t pte_t;
typedef uintptr_t ptep_t;

pte_t get_pte (ptep_t ptep);
pte_t set_pte (ptep_t ptep, pte_t pte);
hal_l1p_t kmap_get_l1p (unsigned long va, int alloc);
hal_l1p_t umap_get_l1p (struct hal_umap *umap, unsigned long va, int alloc);
uaddr_t pt_umap_next (struct hal_umap *umap, uaddr_t uaddr, hal_l1p_t * l1p_out,
		   hal_l1e_t * l1e_out);
void pt_umap_free (struct hal_umap *umap);
void pt_umap_debugwalk (struct hal_umap *umap, unsigned long va);
unsigned long pt_umap_minaddr (void);
unsigned long pt_umap_maxaddr (void);

void tlbflush_global (void);

void serial_init (void);
void serial_putchar (int c);

int vga_putchar (int c);

uint64_t rdmsr (uint32_t ecx);
void wrmsr (uint32_t ecx, uint64_t val);

unsigned long read_cr4 (void);
void write_cr4 (unsigned long r);
unsigned long read_cr3 (void);
void write_cr3 (unsigned long r);

unsigned long frame_bp(struct hal_frame *f);
unsigned long frame_cr2(struct hal_frame *f);

#endif

#endif
