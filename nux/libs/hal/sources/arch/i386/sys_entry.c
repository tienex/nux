/** @file
  i386 System Entry and Exception Frame Management

  Provides exception and interrupt frame handling for i386 architecture.
  Manages system calls, exceptions (including page faults), NMI, and
  hardware interrupts.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <nux/hal.h>
#include <nux/plt.h>

#include "hal/arch/i386/i386.h"
#include "../internal.h"

#if 0
static char *exceptions[] = {
  "Divide by zero exception",
  "Debug exception",
  "NMI",
  "Overflow exception",
  "Breakpoint exception",
  "Bound range exceeded",
  "Invalid opcode",
  "No math coprocessor",
  "Double fault",
  "Coprocessor segment overrun",
  "Invalid TSS",
  "Segment not present",
  "Stack segment fault",
  "General protection fault",
  "Page fault",
  "Reserved exception",
  "Floating point error",
  "Alignment check fault",
  "Machine check fault",
  "SIMD Floating-Point Exception",
};
#endif


/**
  Handle Non-Maskable Interrupt (NMI).

  @param[in] Vect    Exception vector number.
  @param[in] pFrame  Exception frame.

  @return Frame pointer (unchanged).
**/
struct hal_frame *
DoNmi (
  IN uint32_t           Vect,
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
  IN uint32_t           Vect,
  IN struct hal_frame  *pFrame
  )
{
  struct hal_frame *pRf;

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
      unsigned XcptErr;

      /* Page Fault. */

      if (pFrame->err & 1)
	{
	  XcptErr = HAL_PF_REASON_PROT;
	}
      else
	{
	  XcptErr = HAL_PF_REASON_NOTP;
	}

      if (pFrame->err & 2)
	XcptErr |= HAL_PF_INFO_WRITE;

      if (pFrame->err & 4)
	XcptErr |= HAL_PF_INFO_USER;

      if (pFrame->err & 16)
	XcptErr |= HAL_PF_INFO_EXE;

      pRf = hal_entry_pf (pFrame, pFrame->cr2, XcptErr);
    }
  else
    {
      pRf = hal_entry_xcpt (pFrame, Vect);
    }
  return pRf;
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
  struct hal_frame *pRf;

  assert (pFrame->cs == UCS);

  pRf =
    hal_entry_syscall (pFrame, pFrame->eax, pFrame->edi, pFrame->esi, pFrame->ecx, pFrame->edx, pFrame->ebx,
		       pFrame->ebp);
  return pRf;
}

/**
  Handle hardware interrupt vector.

  @param[in] Vect    Interrupt vector number.
  @param[in] pFrame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoVector (
  IN uint32_t           Vect,
  IN struct hal_frame  *pFrame
  )
{
  return plt_interrupt (Vect, pFrame);
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
  pFrame->eip = 0;
  pFrame->esp = 0;

  pFrame->cs = UCS;
  pFrame->ds = UDS;
  pFrame->es = UDS;
  pFrame->fs = UDS;
  pFrame->gs = UDS;
  pFrame->ss = UDS;

  pFrame->eflags = 0x202;
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
  return pFrame->cs == UCS;
}

/**
  Get instruction pointer from frame.

  @param[in] pFrame  Exception frame.

  @return Instruction pointer value.
**/
vaddr_t
HalFrameGetIp (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->eip;
}

/**
  Set instruction pointer in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Ip      Instruction pointer value.
**/
VOID
HalFrameSetIp (
  IN OUT struct hal_frame  *pFrame,
  IN     vaddr_t           Ip
  )
{
  pFrame->eip = Ip;
}

/**
  Get stack pointer from frame.

  @param[in] pFrame  Exception frame.

  @return Stack pointer value.
**/
vaddr_t
HalFrameGetSp (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->esp;
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
  pFrame->esp = Sp;
}

/**
  Get global pointer from frame.

  i386 does not use a global pointer, returns 0.

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

  i386 does not use a global pointer, this is a no-op.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Gp      Global pointer value (ignored).
**/
VOID
HalFrameSetGp (
  IN OUT struct hal_frame  *pFrame,
  IN     unsigned long     Gp
  )
{
  /* Do nothing. */
}

/**
  Set thread-local storage pointer in frame.

  i386 TLS requires setting an LDT for each process and is very different
  from modern architectures. For now, this is a no-op.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Tls     TLS pointer value (ignored).
**/
VOID
HalFrameSetTls (
  IN OUT struct hal_frame  *pFrame,
  IN     unsigned long     Tls
  )
{
  /*
    i386 TLS requires setting a LDT for each process and it is in
    general very different to what modern architectures do.

    It is possible to add TLS support to the i386 HAL. For now, we
    just ignore the call.
  */
}

/**
  Set argument register A0 (EAX) in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     A0      Value for first argument.
**/
VOID
HalFrameSetA0 (
  IN OUT struct hal_frame  *pFrame,
  IN     unsigned long     A0
  )
{
  pFrame->eax = A0;
}

/**
  Set argument register A1 (EDX) in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     A1      Value for second argument.
**/
VOID
HalFrameSetA1 (
  IN OUT struct hal_frame  *pFrame,
  IN     unsigned long     A1
  )
{
  pFrame->edx = A1;
}

/**
  Set argument register A2 (ECX) in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     A2      Value for third argument.
**/
VOID
HalFrameSetA2 (
  IN OUT struct hal_frame  *pFrame,
  IN     unsigned long     A2
  )
{
  pFrame->ecx = A2;
}

/**
  Set return value in frame.

  @param[in,out] pFrame  Exception frame.
  @param[in]     Ret     Return value (stored in EAX).
**/
VOID
HalFrameSetRet (
  IN OUT struct hal_frame  *pFrame,
  IN     unsigned long     Ret
  )
{
  pFrame->eax = Ret;
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

  hallog ("EAX: %08x EBX: %08x ECX: %08x EDX:%08x",
	  pFrame->eax, pFrame->ebx, pFrame->ecx, pFrame->edx);
  hallog ("EDI: %08x ESI: %08x EBP: %08x ESP:%08x",
	  pFrame->edi, pFrame->esi, pFrame->ebp, pFrame->esp);
  hallog (" CS: %04x     EIP: %08x EFL: %08x",
	  (int) pFrame->cs, pFrame->eip, pFrame->eflags);
  hallog (" DS: %04x      ES: %04x     FS: %04x      GS: %04x",
	  pFrame->ds, pFrame->es, pFrame->fs, pFrame->gs);
  hallog ("CR3: %08x CR2: %08x err: %08x", pFrame->cr3, pFrame->cr2, pFrame->err);
}

/**
  Get base pointer from frame.

  @param[in] pFrame  Exception frame.

  @return Base pointer (EBP) value.
**/
unsigned long
FrameBp (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->ebp;
}

/**
  Get CR2 (page fault address) from frame.

  @param[in] pFrame  Exception frame.

  @return CR2 register value.
**/
unsigned long
FrameCr2 (
  IN struct hal_frame  *pFrame
  )
{
  return pFrame->cr2;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use DoNmi instead **/
struct hal_frame * do_nmi (uint32_t vect, struct hal_frame *f) {
  return DoNmi (vect, f);
}

/** @deprecated Use DoException instead **/
struct hal_frame * do_xcpt (uint32_t vect, struct hal_frame *f) {
  return DoException (vect, f);
}

/** @deprecated Use DoSyscall instead **/
struct hal_frame * do_syscall (struct hal_frame *f) {
  return DoSyscall (f);
}

/** @deprecated Use DoVector instead **/
struct hal_frame * do_vect (uint32_t vect, struct hal_frame *f) {
  return DoVector (vect, f);
}

/** @deprecated Use HalFrameInitialize instead **/
void hal_frame_init (struct hal_frame *f) {
  HalFrameInitialize (f);
}

/** @deprecated Use HalFrameIsUser instead **/
bool hal_frame_isuser (struct hal_frame *f) {
  return HalFrameIsUser (f);
}

/** @deprecated Use HalFrameGetIp instead **/
vaddr_t hal_frame_getip (struct hal_frame *f) {
  return HalFrameGetIp (f);
}

/** @deprecated Use HalFrameSetIp instead **/
void hal_frame_setip (struct hal_frame *f, vaddr_t ip) {
  HalFrameSetIp (f, ip);
}

/** @deprecated Use HalFrameGetSp instead **/
vaddr_t hal_frame_getsp (struct hal_frame *f) {
  return HalFrameGetSp (f);
}

/** @deprecated Use HalFrameSetSp instead **/
void hal_frame_setsp (struct hal_frame *f, vaddr_t sp) {
  HalFrameSetSp (f, sp);
}

/** @deprecated Use HalFrameGetGp instead **/
vaddr_t hal_frame_getgp (struct hal_frame *f) {
  return HalFrameGetGp (f);
}

/** @deprecated Use HalFrameSetGp instead **/
void hal_frame_setgp (struct hal_frame *f, unsigned long gp) {
  HalFrameSetGp (f, gp);
}

/** @deprecated Use HalFrameSetTls instead **/
void hal_frame_settls (struct hal_frame *f, unsigned long tls) {
  HalFrameSetTls (f, tls);
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
