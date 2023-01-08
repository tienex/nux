#ifndef _APXH_X86_H
#define _APXH_X86_H

/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	GPL2.0+
*/

#define MSR_IA32_MISC_ENABLE 0x000001a0
#define _MSR_IA32_MISC_ENABLE_XD_DISABLE (1LL << 34)

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_NXE (1LL << 11)
#define _MSR_IA32_EFER_LME (1LL << 8)

#define CR4_PAE (1 << 5)

#define CR0_PG  (1 << 31)
#define CR0_WP  (1 << 16)

static inline unsigned long
read_cr4 (void)
{
  unsigned long reg;

  asm volatile ("mov %%cr4, %0\n":"=r" (reg));
  return reg;
}

static inline void
write_cr4 (unsigned long reg)
{
  asm volatile ("mov %0, %%cr4\n"::"r" (reg));
}

static inline unsigned long
read_cr3 (void)
{
  unsigned long reg;

  asm volatile ("mov %%cr3, %0\n":"=r" (reg));
  return reg;
}

static inline void
write_cr3 (unsigned long reg)
{
  asm volatile ("mov %0, %%cr3\n"::"r" (reg));
}

static inline unsigned long
read_cr0 (void)
{
  unsigned long reg;

  asm volatile ("mov %%cr0, %0\n":"=r" (reg));
  return reg;
}

static inline void
write_cr0 (unsigned long reg)
{
  asm volatile ("mov %0, %%cr0\n"::"r" (reg));
}

static inline void
cpuid (uint32_t * eax, uint32_t * ebx, uint32_t * ecx, uint32_t * edx)
{
  asm volatile ("cpuid\n":"+a" (*eax), "=b" (*ebx), "+c" (*ecx), "=d" (*edx));
}

static inline uint64_t
rdmsr (uint32_t ecx)
{
  uint32_t edx, eax;


  asm volatile ("rdmsr\n":"=d" (edx), "=a" (eax):"c" (ecx));

  return ((uint64_t) edx << 32) | eax;
}

static inline void
wrmsr (uint32_t ecx, uint64_t msr)
{
  uint32_t edx, eax;

  eax = (uint32_t) msr;
  edx = msr >> 32;

  asm volatile ("wrmsr\n"::"c" (ecx), "d" (edx), "a" (eax));
}

static inline void
lgdt (uintptr_t ptr)
{
  asm volatile ("lgdtl (%0)\n"::"r" (ptr));
}

#endif
