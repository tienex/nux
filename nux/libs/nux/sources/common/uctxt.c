/** @file
  NUX User Context Management

  Provides management of user execution contexts, including register
  manipulation, context initialization, and frame conversions. User
  contexts can represent active user frames, idle states, or invalid
  kernel-only contexts.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <assert.h>
#include <nux/nux.h>

#include <nux/internal.h>

/**
  Get user context from HAL frame.

  Determines the user context type based on frame state and CPU
  idle status. Returns user frame context, idle context, or invalid.

  @param[in] Frame  HAL frame to examine.

  @return User context pointer, UCTXT_IDLE if coming from idle, or
          UCTXT_INVALID if kernel-only context.
**/
UCTXT *
UctxtGet (
  IN struct hal_frame  *Frame
  )
{
  BOOLEAN WasIdle = CpuWasIdle ();

  if (hal_frame_isuser (Frame))
    {
      assert (!WasIdle);
      return (UCTXT *) Frame;
    }
  else if (WasIdle)
    {
      CpuClearIdle ();
      return UCTXT_IDLE;
    }
  else
    {
      return UCTXT_INVALID;
    }
}

/**
  Get user context from frame (fatal if invalid).

  Similar to UctxtGet() but terminates with fatal error if the
  context is invalid (kernel-only).

  @param[in] Frame  HAL frame to examine.

  @return User context pointer or UCTXT_IDLE. Never returns UCTXT_INVALID.
**/
UCTXT *
UctxtGetUser (
  IN struct hal_frame  *Frame
  )
{
  UCTXT *Uctxt;

  Uctxt = UctxtGet (Frame);

  if (Uctxt == UCTXT_INVALID)
    {
      fatal ("Expected User Frame.");
    }

  return Uctxt;
}

/**
  Get HAL frame pointer from user context.

  Converts user context to HAL frame pointer, or NULL if context
  is invalid or idle.

  @param[in] Uctxt  User context to convert.

  @return HAL frame pointer, or NULL if context is special.
**/
struct hal_frame *
UctxtFramePointer (
  IN UCTXT  *Uctxt
  )
{
  if (Uctxt != UCTXT_INVALID && Uctxt != UCTXT_IDLE)
    return (struct hal_frame *) Uctxt;

  return NULL;
}

/**
  Get HAL frame from user context.

  Converts user context to HAL frame, entering idle state if
  context is UCTXT_IDLE. Asserts if context is invalid.

  @param[in] Uctxt  User context to convert.

  @return HAL frame pointer. May not return if context is UCTXT_IDLE.
**/
struct hal_frame *
UctxtFrame (
  IN UCTXT  *Uctxt
  )
{
  assert (Uctxt != UCTXT_INVALID);

  if (Uctxt == UCTXT_IDLE)
    {
      CpuIdle ();
    }
  else
    {
      return (struct hal_frame *) Uctxt;
    }
}

/**
  Initialize user context.

  Sets up a user context with specified instruction pointer,
  stack pointer, and global pointer values.

  @param[in] Uctxt  User context to initialize.
  @param[in] Ip      Instruction pointer value.
  @param[in] Sp      Stack pointer value.
  @param[in] Gp      Global pointer value.
**/
VOID
UctxtInitialize (
  IN UCTXT  *Uctxt,
  IN VIRTUAL_ADDRESS  Ip,
  IN VIRTUAL_ADDRESS  Sp,
  IN VIRTUAL_ADDRESS  Gp
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);

  hal_frame_init (Frame);
  hal_frame_setip (Frame, Ip);
  hal_frame_setsp (Frame, Sp);
  hal_frame_setgp (Frame, Gp);
}

/**
  Get instruction pointer from user context.

  @param[in] Uctxt  User context to query.

  @return Instruction pointer value.
**/
VIRTUAL_ADDRESS
UctxtGetIp (
  IN UCTXT  *Uctxt
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  return hal_frame_getip (Frame);
}

/**
  Set instruction pointer in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] Ip      New instruction pointer value.
**/
VOID
UctxtSetIp (
  IN UCTXT  *Uctxt,
  IN VIRTUAL_ADDRESS  Ip
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_setip (Frame, Ip);
}

/**
  Get stack pointer from user context.

  @param[in] Uctxt  User context to query.

  @return Stack pointer value.
**/
VIRTUAL_ADDRESS
UctxtGetSp (
  IN UCTXT  *Uctxt
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  return hal_frame_getsp (Frame);
}

/**
  Set stack pointer in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] Sp      New stack pointer value.
**/
VOID
UctxtSetSp (
  IN UCTXT  *Uctxt,
  IN VIRTUAL_ADDRESS  Sp
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_setsp (Frame, Sp);
}

/**
  Get global pointer from user context.

  @param[in] Uctxt  User context to query.

  @return Global pointer value.
**/
VIRTUAL_ADDRESS
UctxtGetGp (
  IN UCTXT  *Uctxt
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  return hal_frame_getgp (Frame);
}

/**
  Set global pointer in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] Gp      New global pointer value.
**/
VOID
UctxtSetGp (
  IN UCTXT  *Uctxt,
  IN VIRTUAL_ADDRESS  Gp
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_setgp (Frame, Gp);
}

/**
  Set return value in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] Ret     Return value to set.
**/
VOID
UctxtSetRet (
  IN UCTXT      *Uctxt,
  IN UINTN  Ret
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_setret (Frame, Ret);
}

/**
  Set argument 0 in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] A0      Argument 0 value.
**/
VOID
UctxtSetA0 (
  IN UCTXT      *Uctxt,
  IN UINTN  A0
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_seta0 (Frame, A0);
}

/**
  Set argument 1 in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] A1      Argument 1 value.
**/
VOID
UctxtSetA1 (
  IN UCTXT      *Uctxt,
  IN UINTN  A1
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_seta1 (Frame, A1);
}

/**
  Set argument 2 in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] A2      Argument 2 value.
**/
VOID
UctxtSetA2 (
  IN UCTXT      *Uctxt,
  IN UINTN  A2
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_seta2 (Frame, A2);
}

/**
  Set TLS pointer in user context.

  @param[in] Uctxt  User context to modify.
  @param[in] Tls     TLS pointer value.
**/
VOID
UctxtSetTls (
  IN UCTXT      *Uctxt,
  IN UINTN  Tls
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);
  assert (Frame);
  hal_frame_settls (Frame, Tls);
}

/**
  Print user context information.

  Outputs context state to console. Handles special contexts
  (invalid/idle) appropriately.

  @param[in] Uctxt  User context to print.
**/
VOID
UctxtPrint (
  IN UCTXT  *Uctxt
  )
{
  struct hal_frame *Frame = UctxtFramePointer (Uctxt);

  switch ((UINTN) Frame)
    {
    case 0:
      info ("INVALID/IDLE FRAME");
      break;
    default:
      hal_frame_print (Frame);
    }
}

/**
  Bootstrap user context with entry point.

  Initializes user context with the user entry point from HAL.
  Returns FALSE if no user entry point is available.

  @param[in] Uctxt  User context to bootstrap.

  @retval TRUE   Context bootstrapped successfully.
  @retval FALSE  No user entry point available.
**/
BOOLEAN
UctxtBootstrap (
  IN UCTXT  *Uctxt
  )
{
  VIRTUAL_ADDRESS UserEntry;

  UserEntry = hal_virtmem_userentry ();
  if (UserEntry == 0)
    {
      return FALSE;
    }

  UctxtInitialize (Uctxt, UserEntry, 0, 0);
  return TRUE;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use UctxtGet instead **/
UCTXT *UctxtGet (struct hal_frame *f) {
  return UctxtGet (f);
}

/** @deprecated Use UctxtGetUser instead **/
UCTXT *UctxtGetUser (struct hal_frame *f) {
  return UctxtGetUser (f);
}

/** @deprecated Use UctxtFramePointer instead **/
struct hal_frame *uctxt_frame_pointer (UCTXT * uctxt) {
  return UctxtFramePointer (uctxt);
}

/** @deprecated Use UctxtFrame instead **/
struct hal_frame *UctxtFrame (UCTXT * uctxt) {
  return UctxtFrame (uctxt);
}

/** @deprecated Use UctxtInitialize instead **/
void UctxtInit (UCTXT * uctxt, VIRTUAL_ADDRESS ip, VIRTUAL_ADDRESS sp, VIRTUAL_ADDRESS gp) {
  UctxtInitialize (uctxt, ip, sp, gp);
}

/** @deprecated Use UctxtGetIp instead **/
VIRTUAL_ADDRESS UctxtGetIp (UCTXT * uctxt) {
  return UctxtGetIp (uctxt);
}

/** @deprecated Use UctxtSetIp instead **/
void UctxtSetIp (UCTXT * uctxt, VIRTUAL_ADDRESS ip) {
  UctxtSetIp (uctxt, ip);
}

/** @deprecated Use UctxtGetSp instead **/
VIRTUAL_ADDRESS UctxtGetSp (UCTXT * uctxt) {
  return UctxtGetSp (uctxt);
}

/** @deprecated Use UctxtSetSp instead **/
void UctxtSetSp (UCTXT * uctxt, VIRTUAL_ADDRESS sp) {
  UctxtSetSp (uctxt, sp);
}

/** @deprecated Use UctxtGetGp instead **/
VIRTUAL_ADDRESS UctxtGetGp (UCTXT * uctxt) {
  return UctxtGetGp (uctxt);
}

/** @deprecated Use UctxtSetGp instead **/
void UctxtSetGp (UCTXT * uctxt, VIRTUAL_ADDRESS gp) {
  UctxtSetGp (uctxt, gp);
}

/** @deprecated Use UctxtSetRet instead **/
void UctxtSetRet (UCTXT * uctxt, unsigned long ret) {
  UctxtSetRet (uctxt, ret);
}

/** @deprecated Use UctxtSetA0 instead **/
void UctxtSetA0 (UCTXT * uctxt, unsigned long a0) {
  UctxtSetA0 (uctxt, a0);
}

/** @deprecated Use UctxtSetA1 instead **/
void UctxtSetA1 (UCTXT * uctxt, unsigned long a1) {
  UctxtSetA1 (uctxt, a1);
}

/** @deprecated Use UctxtSetA2 instead **/
void UctxtSetA2 (UCTXT * uctxt, unsigned long a2) {
  UctxtSetA2 (uctxt, a2);
}

/** @deprecated Use UctxtSetTls instead **/
void UctxtSetTls (UCTXT * uctxt, unsigned long tls) {
  UctxtSetTls (uctxt, tls);
}

/** @deprecated Use UctxtPrint instead **/
void UctxtPrint (UCTXT * uctxt) {
  UctxtPrint (uctxt);
}

/** @deprecated Use UctxtBootstrap instead **/
BOOLEAN UctxtBootstrap (UCTXT * uctxt) {
  return UctxtBootstrap (uctxt);
}
