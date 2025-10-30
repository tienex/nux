/** @file
  AMD64 Exception Frame Management

  Provides exception and interrupt frame handling for AMD64 architecture.
  Manages system calls, exceptions (including page faults), NMI, and
  hardware interrupts.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <hal/hal.h>
#include <platform/platform.h>

#include <hal/arch/amd64/amd64.h>
#include <hal/internal.h>

/**
  Check if address is canonical (valid for AMD64).

  @param[in] Addr  Address to check.

  @retval TRUE   Address is canonical.
  @retval FALSE  Address is non-canonical.
**/
static inline BOOLEAN
IsCanonical (
  IN UINT64  Addr
  )
{
  return ((UINT64) ((INT64) Addr << 16) >> 16) == Addr;
}

/**
  Handle system call.

  @param[in] pFrame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoSyscall (
  IN struct hal_frame  *pFrame
  )
{
  return hal_entry_syscall (pFrame, pFrame->intr.rax, pFrame->intr.rdi, pFrame->intr.rsi,
			    pFrame->intr.rdx, pFrame->intr.rbx, pFrame->intr.r8, pFrame->intr.r9);
}

/**
  System call entry point.

  Validates return address is canonical to avoid Intel CPU issue with
  #GP on user stack.

  @param[in] pFrame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoSyscallEntry (
  IN struct hal_frame  *pFrame
  )
{
  struct hal_frame *pRetFrame;

  assert (pFrame->type == FRAMETYPE_SYSC);

  pRetFrame = DoSyscall (pFrame);

  if (!IsCanonical (pRetFrame->intr.rip))
    {
      /*
         This is problematic on Intel. #GP will run in kernel mode on
         user stack. Get the #GP from iret.
       */
      pRetFrame->type = FRAMETYPE_INTR;
    }

  return pRetFrame;
}

/**
  Handle Non-Maskable Interrupt (NMI).

  @param[in] pFrame  Exception frame.

  @return Frame pointer (unchanged).
**/
struct hal_frame *
DoNmi (
  IN struct hal_frame  *pFrame
  )
{

  hal_entry_nmi (pFrame);
  return pFrame;
}

/**
  Handle CPU exception.

  Processes various CPU exceptions including double fault, machine check,
  and page faults.

  @param[in] Vect    Exception vector number.
  @param[in] pFrame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoException (
  IN UINT64            Vect,
  IN struct hal_frame  *pFrame
  )
{
  struct hal_frame *pRetFrame;

  if (Vect == 8)
    {
      nux_panic ("DOUBLE FAULT EXCEPTION:\n", pFrame);
    }
  else if (Vect == 18)
    {
      nux_panic ("MACHINE CHECK EXCEPTION:\n", pFrame);
    }
  else if (Vect == 14)
    {
      UINT32 XcptErr;

      /* Page Fault. */

      if (pFrame->intr.err & 1)
	{
	  XcptErr = HAL_PF_REASON_PROT;
	}
      else
	{
	  XcptErr = HAL_PF_REASON_NOTP;
	}

      if (pFrame->intr.err & 2)
	XcptErr |= HAL_PF_INFO_WRITE;

      if (pFrame->intr.err & 4)
	XcptErr |= HAL_PF_INFO_USER;

      if (pFrame->intr.err & 16)
	XcptErr |= HAL_PF_INFO_EXE;
      pRetFrame = hal_entry_pf (pFrame, pFrame->intr.cr2, XcptErr);
    }
  else
    {
      pRetFrame = hal_entry_xcpt (pFrame, Vect);
    }

  return pRetFrame;
}

/**
  Handle hardware interrupt vector.

  @param[in] Vect    Interrupt vector number.
  @param[in] pFrame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoVector (
  IN UINT64            Vect,
  IN struct hal_frame  *pFrame
  )
{
  return plt_interrupt (Vect, pFrame);
}

/**
  Interrupt entry point.

  Dispatches to appropriate handler based on vector number.

  @param[in] pFrame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoInterruptEntry (
  IN struct hal_frame  *pFrame
  )
{
  UINT64 Vect;
  struct hal_frame *pRetFrame;

  assert (pFrame->type == FRAMETYPE_INTR);

  Vect = pFrame->intr.vect;

  if (Vect == VECT_SYSC)
    pRetFrame = DoSyscall (pFrame);
  else if (Vect == 2)
    pRetFrame = DoNmi (pFrame);
  else if (Vect < 32)
    pRetFrame = DoException (Vect, pFrame);
  else
    pRetFrame = DoVector (Vect, pFrame);

  return pRetFrame;
}

/**
  Initialize an exception frame.

  Sets up initial frame state for user mode with interrupts enabled.

  @param[out] pFrame  Exception frame to initialize.
**/
VOID
HalFrameInitialize (
  OUT struct hal_frame  *pFrame
  )
{
  memset (pFrame, 0, sizeof (*pFrame));
  pFrame->type = FRAMETYPE_INTR;
  pFrame->intr.cs = UCS;
  pFrame->intr.rflags = 0x202;
  pFrame->intr.ss = UDS;
}

/**
  Check if frame is from user mode.

  @param[in] pFrame  Exception frame to check.

  @retval TRUE   Frame is from user mode.
  @retval FALSE  Frame is from supervisor mode.
**/
BOOLEAN
HalFrameIsUser (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->intr.cs == UCS;
}

/**
  Set instruction pointer in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Ip      Instruction pointer value.
**/
VOID
HalFrameSetIp (
  IN OUT struct hal_frame  *pFrame,
  IN     UINTN             Ip
  )
{
  pFrame->intr.rip = Ip;
}

/**
  Get instruction pointer from frame.

  @param[in] pFrame  Exception frame.

  @return Instruction pointer value.
**/
UINTN
HalFrameGetIp (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->intr.rip;
}

/**
  Set stack pointer in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Sp      Stack pointer value.
**/
VOID
HalFrameSetSp (
  IN OUT struct hal_frame  *pFrame,
  IN     vaddr_t           Sp
  )
{
  pFrame->intr.rsp = Sp;
}

/**
  Get stack pointer from frame.

  @param[in] pFrame  Exception frame.

  @return Stack pointer value.
**/
UINTN
HalFrameGetSp (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->intr.rsp;
}

/**
  Get global pointer from frame.

  AMD64 does not use a global pointer, returns 0.

  @param[in] pFrame  Exception frame.

  @return Always returns 0.
**/
vaddr_t
HalFrameGetGp (
  IN struct hal_frame  *pFrame
  )
{
  return 0;
}

/**
  Set global pointer in frame.

  AMD64 does not use a global pointer, this is a no-op.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Gp      Global pointer value (ignored).
**/
VOID
HalFrameSetGp (
  IN OUT struct hal_frame  *pFrame,
  IN     UINTN             Gp
  )
{
  /* Do nothing. */
}

/**
  Set argument register A0 (RDI) in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     A0      Value for first argument.
**/
VOID
HalFrameSetA0 (
  IN OUT struct hal_frame  *pFrame,
  IN     UINTN             A0
  )
{
  pFrame->intr.rdi = A0;
}

/**
  Set argument register A1 (RSI) in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     A1      Value for second argument.
**/
VOID
HalFrameSetA1 (
  IN OUT struct hal_frame  *pFrame,
  IN     UINTN             A1
  )
{
  pFrame->intr.rsi = A1;
}

/**
  Set argument register A2 (RDX) in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     A2      Value for third argument.
**/
VOID
HalFrameSetA2 (
  IN OUT struct hal_frame  *pFrame,
  IN     UINTN             A2
  )
{
  pFrame->intr.rdx = A2;
}

/**
  Set return value in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Ret     Return value (stored in RAX).
**/
VOID
HalFrameSetRet (
  IN OUT struct hal_frame  *pFrame,
  IN     UINTN             Ret
  )
{
  pFrame->intr.rax = Ret;
}

/**
  Set thread-local storage base in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Tls     TLS base address (stored in FS.base).
**/
VOID
HalFrameSetTls (
  IN OUT struct hal_frame  *pFrame,
  IN     UINTN             Tls
  )
{
  printf ("Setting fbase to %lx\n", Tls);
  pFrame->intr.fsbase = Tls;
}

/**
  Print exception frame contents.

  Displays all register values from the exception frame for debugging.

  @param[in] pFrame  Exception frame to print.
**/
VOID
HalFramePrint (
  IN struct hal_frame  *pFrame
  )
{
  hallog ("RAX: %016lx RBX: %016lx\nRCX: %016lx RDX: %016lx",
	  pFrame->intr.rax, pFrame->intr.rbx, pFrame->intr.rcx, pFrame->intr.rdx);
  hallog ("RDI: %016lx RSI: %016lx\nRBP: %016lx RSP: %016lx",
	  pFrame->intr.rdi, pFrame->intr.rsi, pFrame->intr.rbp, pFrame->intr.rsp);
  hallog ("R8 : %016lx R9 : %016lx\nR10: %016lx R11: %016lx",
	  pFrame->intr.r8, pFrame->intr.r9, pFrame->intr.r10, pFrame->intr.r11);
  hallog ("R12: %016lx R13: %016lx\nR14: %016lx R15: %016lx",
	  pFrame->intr.r12, pFrame->intr.r13, pFrame->intr.r14, pFrame->intr.r15);
  hallog ("GS:  %016lx FS:  %016lx", pFrame->intr.gsbase, pFrame->intr.fsbase);
  hallog (" CS: %04x     RIP: %016lx RFL: %016lx",
	  (INT32) pFrame->intr.cs, pFrame->intr.rip, pFrame->intr.rflags);
  hallog (" SS: %04x     RSP: %016lx", (INT32) pFrame->intr.ss, pFrame->intr.rsp);
  hallog ("CR2: %016lx err: %08lx", pFrame->intr.cr2, pFrame->intr.err);
}

/**
  Get base pointer from frame.

  @param[in] pFrame  Exception frame.

  @return Base pointer (RBP) value.
**/
UINTN
FrameBp (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->intr.rbp;
}

/**
  Get CR2 (page fault address) from frame.

  @param[in] pFrame  Exception frame.

  @return CR2 register value.
**/
UINTN
FrameCr2 (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->intr.cr2;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use IsCanonical instead **/
static inline bool is_canonical (uint64_t addr) {
  return IsCanonical (addr);
}

/** @deprecated Use DoSyscall instead **/
struct hal_frame *do_syscall (struct hal_frame *f) {
  return DoSyscall (f);
}

/** @deprecated Use DoSyscallEntry instead **/
struct hal_frame *do_syscall_entry (struct hal_frame *f) {
  return DoSyscallEntry (f);
}

/** @deprecated Use DoNmi instead **/
struct hal_frame *do_nmi (struct hal_frame *f) {
  return DoNmi (f);
}

/** @deprecated Use DoException instead **/
struct hal_frame *do_xcpt (uint64_t vect, struct hal_frame *f) {
  return DoException (vect, f);
}

/** @deprecated Use DoVector instead **/
struct hal_frame *do_vect (uint64_t vect, struct hal_frame *f) {
  return DoVector (vect, f);
}

/** @deprecated Use DoInterruptEntry instead **/
struct hal_frame *do_intr_entry (struct hal_frame *f) {
  return DoInterruptEntry (f);
}

/** @deprecated Use HalFrameInitialize instead **/
void hal_frame_init (struct hal_frame *f) {
  HalFrameInitialize (f);
}

/** @deprecated Use HalFrameIsUser instead **/
bool hal_frame_isuser (struct hal_frame *f) {
  return HalFrameIsUser (f);
}

/** @deprecated Use HalFrameSetIp instead **/
void hal_frame_setip (struct hal_frame *f, unsigned long ip) {
  HalFrameSetIp (f, ip);
}

/** @deprecated Use HalFrameGetIp instead **/
unsigned long hal_frame_getip (struct hal_frame *f) {
  return HalFrameGetIp (f);
}

/** @deprecated Use HalFrameSetSp instead **/
void hal_frame_setsp (struct hal_frame *f, vaddr_t sp) {
  HalFrameSetSp (f, sp);
}

/** @deprecated Use HalFrameGetSp instead **/
unsigned long hal_frame_getsp (struct hal_frame *f) {
  return HalFrameGetSp (f);
}

/** @deprecated Use HalFrameGetGp instead **/
vaddr_t hal_frame_getgp (struct hal_frame *f) {
  return HalFrameGetGp (f);
}

/** @deprecated Use HalFrameSetGp instead **/
void hal_frame_setgp (struct hal_frame *f, unsigned long gp) {
  HalFrameSetGp (f, gp);
}

/** @deprecated Use HalFrameSetA0 instead **/
void hal_frame_seta0 (struct hal_frame *f, unsigned long a0) {
  HalFrameSetA0 (f, a0);
}

/** @deprecated Use HalFrameSetA1 instead **/
void hal_frame_seta1 (struct hal_frame *f, unsigned long a1) {
  HalFrameSetA1 (f, a1);
}

/** @deprecated Use HalFrameSetA2 instead **/
void hal_frame_seta2 (struct hal_frame *f, unsigned long a2) {
  HalFrameSetA2 (f, a2);
}

/** @deprecated Use HalFrameSetRet instead **/
void hal_frame_setret (struct hal_frame *f, unsigned long r) {
  HalFrameSetRet (f, r);
}

/** @deprecated Use HalFrameSetTls instead **/
void hal_frame_settls (struct hal_frame *f, unsigned long tls) {
  HalFrameSetTls (f, tls);
}

/** @deprecated Use HalFramePrint instead **/
void hal_frame_print (struct hal_frame *f) {
  HalFramePrint (f);
}

/** @deprecated Use FrameBp instead **/
unsigned long frame_bp (struct hal_frame *f) {
  return FrameBp (f);
}

/** @deprecated Use FrameCr2 instead **/
unsigned long frame_cr2 (struct hal_frame *f) {
  return FrameCr2 (f);
}
