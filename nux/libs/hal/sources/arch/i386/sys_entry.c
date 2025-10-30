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
#include <hal/hal.h>
#include <platform/platform.h>

#include <hal/arch/i386/i386.h>
#include <hal/internal.h>

#if 0
static CHAR8 *exceptions[] = {
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
  @param[in] Frame  Exception frame.

  @return Frame pointer (unchanged).
**/
struct hal_frame *
DoNmi (
  IN UINT32           Vect,
  IN struct hal_frame  *Frame
  )
{

  hal_entry_nmi (Frame);
  return Frame;
}

/**
  Handle CPU exception.

  Processes various CPU exceptions including double fault, machine check,
  and page faults.

  @param[in] Vect    Exception vector number.
  @param[in] Frame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoException (
  IN UINT32           Vect,
  IN struct hal_frame  *Frame
  )
{
  struct hal_frame *Rf;

  if (Vect == 8)
    {
      nux_panic ("DOUBLE FAULT EXCEPTION:\n", Frame);
    }
  else if (Vect == 18)
    {
      nux_panic ("MACHINE CHECK EXCEPTION:\n", Frame);
    }
  else if (Vect == 14)
    {
      unsigned XcptErr;

      /* Page Fault. */

      if (Frame->err & 1)
	{
	  XcptErr = HAL_PF_REASON_PROT;
	}
      else
	{
	  XcptErr = HAL_PF_REASON_NOTP;
	}

      if (Frame->err & 2)
	XcptErr |= HAL_PF_INFO_WRITE;

      if (Frame->err & 4)
	XcptErr |= HAL_PF_INFO_USER;

      if (Frame->err & 16)
	XcptErr |= HAL_PF_INFO_EXE;

      Rf = hal_entry_pf (Frame, Frame->cr2, XcptErr);
    }
  else
    {
      Rf = hal_entry_xcpt (Frame, Vect);
    }
  return Rf;
}

/**
  Handle system call.

  @param[in] Frame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoSyscall (
  IN struct hal_frame  *Frame
  )
{
  struct hal_frame *Rf;

  assert (Frame->cs == UCS);

  Rf =
    hal_entry_syscall (Frame, Frame->eax, Frame->edi, Frame->esi, Frame->ecx, Frame->edx, Frame->ebx,
		       Frame->ebp);
  return Rf;
}

/**
  Handle hardware interrupt vector.

  @param[in] Vect    Interrupt vector number.
  @param[in] Frame  Exception frame.

  @return Modified frame pointer.
**/
struct hal_frame *
DoVector (
  IN UINT32           Vect,
  IN struct hal_frame  *Frame
  )
{
  return plt_interrupt (Vect, Frame);
}

/**
  Initialize an exception frame.

  Sets up initial frame state for user mode with interrupts enabled.

  @param[out] Frame  Exception frame to initialize.
**/
VOID
HalFrameInitialize (
  OUT struct hal_frame  *Frame
  )
{
  memset (Frame, 0, sizeof (*Frame));
  Frame->eip = 0;
  Frame->esp = 0;

  Frame->cs = UCS;
  Frame->ds = UDS;
  Frame->es = UDS;
  Frame->fs = UDS;
  Frame->gs = UDS;
  Frame->ss = UDS;

  Frame->eflags = 0x202;
}

/**
  Check if frame is from user mode.

  @param[in] Frame  Exception frame to check.

  @retval TRUE   Frame is from user mode.
  @retval FALSE  Frame is from supervisor mode.
**/
BOOLEAN
HalFrameIsUser (
  IN struct hal_frame  *Frame
  )
{
  return Frame->cs == UCS;
}

/**
  Get instruction pointer from frame.

  @param[in] Frame  Exception frame.

  @return Instruction pointer value.
**/
VIRTUAL_ADDRESS
HalFrameGetIp (
  IN struct hal_frame  *Frame
  )
{
  return Frame->eip;
}

/**
  Set instruction pointer in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Ip      Instruction pointer value.
**/
VOID
HalFrameSetIp (
  IN OUT struct hal_frame  *Frame,
  IN     VIRTUAL_ADDRESS           Ip
  )
{
  Frame->eip = Ip;
}

/**
  Get stack pointer from frame.

  @param[in] Frame  Exception frame.

  @return Stack pointer value.
**/
VIRTUAL_ADDRESS
HalFrameGetSp (
  IN struct hal_frame  *Frame
  )
{
  return Frame->esp;
}

/**
  Set stack pointer in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Sp      Stack pointer value.
**/
VOID
HalFrameSetSp (
  IN OUT struct hal_frame  *Frame,
  IN     VIRTUAL_ADDRESS           Sp
  )
{
  Frame->esp = Sp;
}

/**
  Get global pointer from frame.

  i386 does not use a global pointer, returns 0.

  @param[in] Frame  Exception frame.

  @return Always returns 0.
**/
VIRTUAL_ADDRESS
HalFrameGetGp (
  IN struct hal_frame  *Frame
  )
{
  return 0;
}

/**
  Set global pointer in frame.

  i386 does not use a global pointer, this is a no-op.

  @param[in,out] Frame  Exception frame.
  @param[in]     Gp      Global pointer value (ignored).
**/
VOID
HalFrameSetGp (
  IN OUT struct hal_frame  *Frame,
  IN     unsigned long     Gp
  )
{
  /* Do nothing. */
}

/**
  Set thread-local storage pointer in frame.

  i386 TLS requires setting an LDT for each process and is very different
  from modern architectures. For now, this is a no-op.

  @param[in,out] Frame  Exception frame.
  @param[in]     Tls     TLS pointer value (ignored).
**/
VOID
HalFrameSetTls (
  IN OUT struct hal_frame  *Frame,
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

  @param[in,out] Frame  Exception frame.
  @param[in]     A0      Value for first argument.
**/
VOID
HalFrameSetA0 (
  IN OUT struct hal_frame  *Frame,
  IN     unsigned long     A0
  )
{
  Frame->eax = A0;
}

/**
  Set argument register A1 (EDX) in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     A1      Value for second argument.
**/
VOID
HalFrameSetA1 (
  IN OUT struct hal_frame  *Frame,
  IN     unsigned long     A1
  )
{
  Frame->edx = A1;
}

/**
  Set argument register A2 (ECX) in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     A2      Value for third argument.
**/
VOID
HalFrameSetA2 (
  IN OUT struct hal_frame  *Frame,
  IN     unsigned long     A2
  )
{
  Frame->ecx = A2;
}

/**
  Set return value in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Ret     Return value (stored in EAX).
**/
VOID
HalFrameSetRet (
  IN OUT struct hal_frame  *Frame,
  IN     unsigned long     Ret
  )
{
  Frame->eax = Ret;
}

/**
  Print exception frame contents.

  Displays all register values from the exception frame for debugging.

  @param[in] Frame  Exception frame to print.
**/
VOID
HalFramePrint (
  IN struct hal_frame  *Frame
  )
{

  hallog ("EAX: %08x EBX: %08x ECX: %08x EDX:%08x",
	  Frame->eax, Frame->ebx, Frame->ecx, Frame->edx);
  hallog ("EDI: %08x ESI: %08x EBP: %08x ESP:%08x",
	  Frame->edi, Frame->esi, Frame->ebp, Frame->esp);
  hallog (" CS: %04x     EIP: %08x EFL: %08x",
	  (int) Frame->cs, Frame->eip, Frame->eflags);
  hallog (" DS: %04x      ES: %04x     FS: %04x      GS: %04x",
	  Frame->ds, Frame->es, Frame->fs, Frame->gs);
  hallog ("CR3: %08x CR2: %08x err: %08x", Frame->cr3, Frame->cr2, Frame->err);
}

/**
  Get base pointer from frame.

  @param[in] Frame  Exception frame.

  @return Base pointer (EBP) value.
**/
unsigned long
FrameBp (
  IN struct hal_frame  *Frame
  )
{
  return Frame->ebp;
}

/**
  Get CR2 (page fault address) from frame.

  @param[in] Frame  Exception frame.

  @return CR2 register value.
**/
unsigned long
FrameCr2 (
  IN struct hal_frame  *Frame
  )
{
  return Frame->cr2;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use DoNmi instead **/
struct hal_frame * do_nmi (UINT32 vect, struct hal_frame *f) {
  return DoNmi (vect, f);
}

/** @deprecated Use DoException instead **/
struct hal_frame * do_xcpt (UINT32 vect, struct hal_frame *f) {
  return DoException (vect, f);
}

/** @deprecated Use DoSyscall instead **/
struct hal_frame * do_syscall (struct hal_frame *f) {
  return DoSyscall (f);
}

/** @deprecated Use DoVector instead **/
struct hal_frame * do_vect (UINT32 vect, struct hal_frame *f) {
  return DoVector (vect, f);
}

/** @deprecated Use HalFrameInitialize instead **/
VOID hal_frame_init (struct hal_frame *f) {
  HalFrameInitialize (f);
}

/** @deprecated Use HalFrameIsUser instead **/
BOOLEAN hal_frame_isuser (struct hal_frame *f) {
  return HalFrameIsUser (f);
}

/** @deprecated Use HalFrameGetIp instead **/
VIRTUAL_ADDRESS hal_frame_getip (struct hal_frame *f) {
  return HalFrameGetIp (f);
}

/** @deprecated Use HalFrameSetIp instead **/
VOID hal_frame_setip (struct hal_frame *f, VIRTUAL_ADDRESS ip) {
  HalFrameSetIp (f, ip);
}

/** @deprecated Use HalFrameGetSp instead **/
VIRTUAL_ADDRESS hal_frame_getsp (struct hal_frame *f) {
  return HalFrameGetSp (f);
}

/** @deprecated Use HalFrameSetSp instead **/
VOID hal_frame_setsp (struct hal_frame *f, VIRTUAL_ADDRESS sp) {
  HalFrameSetSp (f, sp);
}

/** @deprecated Use HalFrameGetGp instead **/
VIRTUAL_ADDRESS hal_frame_getgp (struct hal_frame *f) {
  return HalFrameGetGp (f);
}

/** @deprecated Use HalFrameSetGp instead **/
VOID hal_frame_setgp (struct hal_frame *f, unsigned INTN gp) {
  HalFrameSetGp (f, gp);
}

/** @deprecated Use HalFrameSetTls instead **/
VOID hal_frame_settls (struct hal_frame *f, unsigned INTN tls) {
  HalFrameSetTls (f, tls);
}

/** @deprecated Use HalFrameSetA0 instead **/
VOID hal_frame_seta0 (struct hal_frame *f, unsigned INTN a0) {
  HalFrameSetA0 (f, a0);
}

/** @deprecated Use HalFrameSetA1 instead **/
VOID hal_frame_seta1 (struct hal_frame *f, unsigned INTN a1) {
  HalFrameSetA1 (f, a1);
}

/** @deprecated Use HalFrameSetA2 instead **/
VOID hal_frame_seta2 (struct hal_frame *f, unsigned INTN a2) {
  HalFrameSetA2 (f, a2);
}

/** @deprecated Use HalFrameSetRet instead **/
VOID hal_frame_setret (struct hal_frame *f, unsigned INTN r) {
  HalFrameSetRet (f, r);
}

/** @deprecated Use HalFramePrint instead **/
VOID hal_frame_print (struct hal_frame *f) {
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
