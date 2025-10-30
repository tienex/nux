/** @file
  RISC-V Exception Frame Management

  Provides functions to initialize, manipulate, and display RISC-V
  exception/interrupt frames. Manages register state for context switching
  and exception handling.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <hal/internal.h>
#include <string.h>

/**
  Initialize an exception frame.

  Sets up initial frame state with interrupts enabled and user mode.

  @param[out] Frame  Exception frame to initialize.
**/
VOID
HalFrameInitialize (
  OUT struct hal_frame  *Frame
  )
{
  memset (Frame, 0, sizeof (*Frame));
  Frame->sstatus = SSTATUS_SPIE;
  Frame->sie = SIE_USER;
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
  return !(Frame->sstatus & SSTATUS_SPP);
}

/**
  Set instruction pointer in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Ip      Instruction pointer value.
**/
VOID
HalFrameSetIp (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             Ip
  )
{
  Frame->pc = Ip;
}

/**
  Set stack pointer in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Sp      Stack pointer value.
**/
VOID
HalFrameSetSp (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             Sp
  )
{
  Frame->sp = Sp;
}

/**
  Set global pointer in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Gp      Global pointer value.
**/
VOID
HalFrameSetGp (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             Gp
  )
{
  Frame->gp = Gp;
}

/**
  Get global pointer from frame.

  @param[in] Frame  Exception frame.

  @return Global pointer value.
**/
VIRTUAL_ADDRESS
HalFrameGetGp (
  IN struct hal_frame  *Frame
  )
{
  return Frame->gp;
}

/**
  Get instruction pointer from frame.

  @param[in] Frame  Exception frame.

  @return Instruction pointer value.
**/
UINTN
HalFrameGetIp (
  IN struct hal_frame  *Frame
  )
{
  return Frame->pc;
}

/**
  Get stack pointer from frame.

  @param[in] Frame  Exception frame.

  @return Stack pointer value.
**/
UINTN
HalFrameGetSp (
  IN struct hal_frame  *Frame
  )
{
  return Frame->sp;
}

/**
  Set argument register A0 in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     A0      Value for A0 register.
**/
VOID
HalFrameSetA0 (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             A0
  )
{
  Frame->a0 = A0;
}

/**
  Set argument register A1 in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     A1      Value for A1 register.
**/
VOID
HalFrameSetA1 (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             A1
  )
{
  Frame->a1 = A1;
}

/**
  Set argument register A2 in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     A2      Value for A2 register.
**/
VOID
HalFrameSetA2 (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             A2
  )
{
  Frame->a2 = A2;
}

/**
  Set return value in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Ret     Return value (stored in A0).
**/
VOID
HalFrameSetRet (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             Ret
  )
{
  Frame->a0 = Ret;
}

/**
  Set thread-local storage pointer in frame.

  @param[in,out] Frame  Exception frame.
  @param[in]     Tls     TLS pointer value (stored in TP).
**/
VOID
HalFrameSetTls (
  IN OUT struct hal_frame  *Frame,
  IN     UINTN             Tls
  )
{
  Frame->tp = Tls;
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
  info ("   PC: %016lx   SP: %016lx    SIE: %016lx", Frame->pc, Frame->sp, Frame->sie);
  info ("STVAL:%016lx CAUSE: %016lx STATUS: %016lx",
	Frame->stval, Frame->scause, Frame->sstatus);
  info ("   RA: %016lx   GP: %016lx     TP: %016lx", Frame->ra, Frame->gp, Frame->tp);
  info ("   T0: %016lx   T1: %016lx     T2: %016lx", Frame->t0, Frame->t1, Frame->t2);
  info ("   T3: %016lx   T4: %016lx     T5: %016lx", Frame->t3, Frame->t4, Frame->t5);
  info ("   T6: %016lx   FP: %016lx     S1: %016lx", Frame->t6, Frame->fp, Frame->s1);
  info ("   S2: %016lx   S3: %016lx     S4: %016lx", Frame->s2, Frame->s3, Frame->s4);
  info ("   S5: %016lx   S6: %016lx     S7: %016lx", Frame->s5, Frame->s6, Frame->s7);
  info ("   S8: %016lx   S9: %016lx    S10: %016lx", Frame->s8, Frame->s9, Frame->s10);
  info ("  S11: %016lx   A0: %016lx     A1: %016lx", Frame->s11, Frame->a0, Frame->a1);
  info ("   A2: %016lx   A3: %016lx     A4: %016lx", Frame->a2, Frame->a3, Frame->a4);
  info ("   A5: %016lx   A6: %016lx     A7: %016lx", Frame->a5, Frame->a6, Frame->a7);
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use HalFrameInitialize instead **/
VOID hal_frame_init (struct hal_frame *f) {
  HalFrameInitialize (f);
}

/** @deprecated Use HalFrameIsUser instead **/
BOOLEAN hal_frame_isuser (struct hal_frame *f) {
  return HalFrameIsUser (f);
}

/** @deprecated Use HalFrameSetIp instead **/
VOID hal_frame_setip (struct hal_frame *f, unsigned INTN ip) {
  HalFrameSetIp (f, ip);
}

/** @deprecated Use HalFrameSetSp instead **/
VOID hal_frame_setsp (struct hal_frame *f, unsigned INTN sp) {
  HalFrameSetSp (f, sp);
}

/** @deprecated Use HalFrameSetGp instead **/
VOID hal_frame_setgp (struct hal_frame *f, unsigned INTN gp) {
  HalFrameSetGp (f, gp);
}

/** @deprecated Use HalFrameGetGp instead **/
VIRTUAL_ADDRESS hal_frame_getgp (struct hal_frame *f) {
  return HalFrameGetGp (f);
}

/** @deprecated Use HalFrameGetIp instead **/
unsigned long hal_frame_getip (struct hal_frame *f) {
  return HalFrameGetIp (f);
}

/** @deprecated Use HalFrameGetSp instead **/
unsigned long hal_frame_getsp (struct hal_frame *f) {
  return HalFrameGetSp (f);
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
VOID hal_frame_setret (struct hal_frame *f, unsigned INTN ret) {
  HalFrameSetRet (f, ret);
}

/** @deprecated Use HalFrameSetTls instead **/
VOID hal_frame_settls (struct hal_frame *f, unsigned INTN ret) {
  HalFrameSetTls (f, ret);
}

/** @deprecated Use HalFramePrint instead **/
VOID hal_frame_print (struct hal_frame *f) {
  HalFramePrint (f);
}
