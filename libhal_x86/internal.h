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
#define _MSR_IA32_EFER_NXE (1LL << 11)

#define MSR_IA32_GS_BASE 0xc0000101
#define MSR_IA32_KERNEL_GS_BASE 0xc0000102

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


#ifndef _ASSEMBLER

#include <nux/nux.h>

#define haldebug(...) debug(__VA_ARGS__)
#define hallog(...) info(__VA_ARGS__)
#define halwarn(...) warn(__VA_ARGS__)
#define halfatal(...) fatal(__VA_ARGS__)

void x86_init (void);
void i386_init (void);
void amd64_init (void);
void pae64_init (void);
void pmap_init (void);

void set_pte (uint64_t *ptep, uint64_t pte);
hal_l1e_t * get_l1p (void *pmap, unsigned long va, int alloc);

int vga_putchar (int c);

uint64_t rdmsr (uint32_t ecx);
void wrmsr (uint32_t ecx, uint64_t val);

#endif

#endif
