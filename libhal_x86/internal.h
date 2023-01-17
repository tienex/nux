/*
  NUX: A kernel Library.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <nux/hal_config.h>

#define MAXCPUS	        HAL_MAXCPUS

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_SCE (1LL << 0)
#define _MSR_IA32_EFER_NXE (1LL << 11)

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

extern int nux_initialized;

void x86_init (void);
void amd64_init (void);
void pae32_init (void);
void pae64_init (void);
void pmap_init (void);
void i386_init_done (void);
void amd64_init_done (void);

typedef uint64_t pte_t;
typedef uintptr_t ptep_t;

pte_t get_pte (ptep_t ptep);
pte_t set_pte (ptep_t ptep, pte_t pte);
hal_l1p_t get_l1p (struct hal_pmap *pmap, unsigned long va, int alloc);

void tlbflush_global (void);

int vga_putchar (int c);

uint64_t rdmsr (uint32_t ecx);
void wrmsr (uint32_t ecx, uint64_t val);

unsigned long read_cr4 (void);
void write_cr4 (unsigned long r);
unsigned long read_cr3 (void);
void write_cr3 (unsigned long r);

#endif

#endif
