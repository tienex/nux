/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef __apxh_x86_h__
#define __apxh_x86_h__

#define MSR_IA32_MISC_ENABLE 0x000001a0
#define _MSR_IA32_MISC_ENABLE_XD_DISABLE (1LL << 34)

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_NXE (1LL << 11)
#define _MSR_IA32_EFER_LME (1LL << 8)

#define MSR_IA32_PAT 0x00000277
#define _MSR_IA32_PAT_UC 0
#define _MSR_IA32_PAT_WC 1
#define _MSR_IA32_PAT_WB 6

#define CR4_PAE (1 << 5)

#define CR0_PG  (1 << 31)
#define CR0_WP  (1 << 16)

static inline UINTN
read_cr4 (void)
{
  UINTN reg;

  asm volatile ("mov %%cr4, %0\n":"=r" (reg));
  return reg;
}

static inline void
write_cr4 (UINTN reg)
{
  asm volatile ("mov %0, %%cr4\n"::"r" (reg));
}

static inline UINTN
read_cr3 (void)
{
  UINTN reg;

  asm volatile ("mov %%cr3, %0\n":"=r" (reg));
  return reg;
}

static inline void
write_cr3 (UINTN reg)
{
  asm volatile ("mov %0, %%cr3\n"::"r" (reg));
}

static inline UINTN
read_cr0 (void)
{
  UINTN reg;

  asm volatile ("mov %%cr0, %0\n":"=r" (reg));
  return reg;
}

static inline void
write_cr0 (UINTN reg)
{
  asm volatile ("mov %0, %%cr0\n"::"r" (reg));
}

static inline void
cpuid (UINT32 * eax, UINT32 * ebx, UINT32 * ecx, UINT32 * edx)
{
  asm volatile ("cpuid\n":"+a" (*eax), "=b" (*ebx), "+c" (*ecx), "=d" (*edx));
}

static inline UINT64
rdmsr (UINT32 ecx)
{
  UINT32 edx, eax;

  asm volatile ("rdmsr\n":"=d" (edx), "=a" (eax):"c" (ecx));

  return ((UINT64) edx << 32) | eax;
}

static inline void
wrmsr (UINT32 ecx, UINT64 msr)
{
  UINT32 edx, eax;

  eax = (UINT32) msr;
  edx = msr >> 32;

  asm volatile ("wrmsr\n"::"c" (ecx), "d" (edx), "a" (eax));
}

static inline void
lgdt (UINTN ptr)
{
  asm volatile ("lgdtl (%0)\n"::"r" (ptr));
}

#endif
