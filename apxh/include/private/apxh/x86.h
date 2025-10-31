/*
  APXH: An ELF boot-loader.
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#pragma once

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

static INLINE UINTN
ReadCr4 (VOID)
{
  return ANX_CPU_READ_CR4();
}

static INLINE VOID
WriteCr4 (UINTN reg)
{
  ANX_CPU_WRITE_CR4(reg);
}

static INLINE UINTN
ReadCr3 (VOID)
{
  return ANX_CPU_READ_CR3();
}

static INLINE VOID
WriteCr3 (UINTN reg)
{
  ANX_CPU_WRITE_CR3(reg);
}

static INLINE UINTN
ReadCr0 (VOID)
{
  return ANX_CPU_READ_CR0();
}

static INLINE VOID
WriteCr0 (UINTN reg)
{
  ANX_CPU_WRITE_CR0(reg);
}

static INLINE VOID
Cpuid (UINT32 * eax, UINT32 * ebx, UINT32 * ecx, UINT32 * edx)
{
  ANX_CPU_CPUID(eax, ebx, ecx, edx);
}

static INLINE UINT64
Rdmsr (UINT32 ecx)
{
  return ANX_CPU_RDMSR(ecx);
}

static INLINE VOID
Wrmsr (UINT32 ecx, UINT64 msr)
{
  ANX_CPU_WRMSR(ecx, msr);
}

static INLINE VOID
Lgdt (UINTN ptr)
{
  ANX_CPU_LOAD_GDT((VOID*)ptr);
}

