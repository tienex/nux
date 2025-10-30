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

  @param[in] pFrame  HAL frame to examine.

  @return User context pointer, UCTXT_IDLE if coming from idle, or
          UCTXT_INVALID if kernel-only context.
**/
uctxt_t *
UctxtGet (
  IN struct hal_frame  *pFrame
  )
{
  BOOLEAN WasIdle = CpuWasIdle ();

  if (hal_frame_isuser (pFrame))
    {
      assert (!WasIdle);
      return (uctxt_t *) pFrame;
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

  @param[in] pFrame  HAL frame to examine.

  @return User context pointer or UCTXT_IDLE. Never returns UCTXT_INVALID.
**/
uctxt_t *
UctxtGetUser (
  IN struct hal_frame  *pFrame
  )
{
  uctxt_t *pUctxt;

  pUctxt = UctxtGet (pFrame);

  if (pUctxt == UCTXT_INVALID)
    {
      fatal ("Expected User Frame.");
    }

  return pUctxt;
}

/**
  Get HAL frame pointer from user context.

  Converts user context to HAL frame pointer, or NULL if context
  is invalid or idle.

  @param[in] pUctxt  User context to convert.

  @return HAL frame pointer, or NULL if context is special.
**/
struct hal_frame *
UctxtFramePointer (
  IN uctxt_t  *pUctxt
  )
{
  if (pUctxt != UCTXT_INVALID && pUctxt != UCTXT_IDLE)
    return (struct hal_frame *) pUctxt;

  return NULL;
}

/**
  Get HAL frame from user context.

  Converts user context to HAL frame, entering idle state if
  context is UCTXT_IDLE. Asserts if context is invalid.

  @param[in] pUctxt  User context to convert.

  @return HAL frame pointer. May not return if context is UCTXT_IDLE.
**/
struct hal_frame *
UctxtFrame (
  IN uctxt_t  *pUctxt
  )
{
  assert (pUctxt != UCTXT_INVALID);

  if (pUctxt == UCTXT_IDLE)
    {
      CpuIdle ();
    }
  else
    {
      return (struct hal_frame *) pUctxt;
    }
}

/**
  Initialize user context.

  Sets up a user context with specified instruction pointer,
  stack pointer, and global pointer values.

  @param[in] pUctxt  User context to initialize.
  @param[in] Ip      Instruction pointer value.
  @param[in] Sp      Stack pointer value.
  @param[in] Gp      Global pointer value.
**/
VOID
UctxtInitialize (
  IN uctxt_t  *pUctxt,
  IN vaddr_t  Ip,
  IN vaddr_t  Sp,
  IN vaddr_t  Gp
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);

  hal_frame_init (pFrame);
  hal_frame_setip (pFrame, Ip);
  hal_frame_setsp (pFrame, Sp);
  hal_frame_setgp (pFrame, Gp);
}

/**
  Get instruction pointer from user context.

  @param[in] pUctxt  User context to query.

  @return Instruction pointer value.
**/
vaddr_t
UctxtGetIp (
  IN uctxt_t  *pUctxt
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  return hal_frame_getip (pFrame);
}

/**
  Set instruction pointer in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] Ip      New instruction pointer value.
**/
VOID
UctxtSetIp (
  IN uctxt_t  *pUctxt,
  IN vaddr_t  Ip
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_setip (pFrame, Ip);
}

/**
  Get stack pointer from user context.

  @param[in] pUctxt  User context to query.

  @return Stack pointer value.
**/
vaddr_t
UctxtGetSp (
  IN uctxt_t  *pUctxt
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  return hal_frame_getsp (pFrame);
}

/**
  Set stack pointer in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] Sp      New stack pointer value.
**/
VOID
UctxtSetSp (
  IN uctxt_t  *pUctxt,
  IN vaddr_t  Sp
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_setsp (pFrame, Sp);
}

/**
  Get global pointer from user context.

  @param[in] pUctxt  User context to query.

  @return Global pointer value.
**/
vaddr_t
UctxtGetGp (
  IN uctxt_t  *pUctxt
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  return hal_frame_getgp (pFrame);
}

/**
  Set global pointer in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] Gp      New global pointer value.
**/
VOID
UctxtSetGp (
  IN uctxt_t  *pUctxt,
  IN vaddr_t  Gp
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_setgp (pFrame, Gp);
}

/**
  Set return value in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] Ret     Return value to set.
**/
VOID
UctxtSetRet (
  IN uctxt_t      *pUctxt,
  IN UINTN  Ret
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_setret (pFrame, Ret);
}

/**
  Set argument 0 in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] A0      Argument 0 value.
**/
VOID
UctxtSetA0 (
  IN uctxt_t      *pUctxt,
  IN UINTN  A0
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_seta0 (pFrame, A0);
}

/**
  Set argument 1 in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] A1      Argument 1 value.
**/
VOID
UctxtSetA1 (
  IN uctxt_t      *pUctxt,
  IN UINTN  A1
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_seta1 (pFrame, A1);
}

/**
  Set argument 2 in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] A2      Argument 2 value.
**/
VOID
UctxtSetA2 (
  IN uctxt_t      *pUctxt,
  IN UINTN  A2
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_seta2 (pFrame, A2);
}

/**
  Set TLS pointer in user context.

  @param[in] pUctxt  User context to modify.
  @param[in] Tls     TLS pointer value.
**/
VOID
UctxtSetTls (
  IN uctxt_t      *pUctxt,
  IN UINTN  Tls
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);
  assert (pFrame);
  hal_frame_settls (pFrame, Tls);
}

/**
  Print user context information.

  Outputs context state to console. Handles special contexts
  (invalid/idle) appropriately.

  @param[in] pUctxt  User context to print.
**/
VOID
UctxtPrint (
  IN uctxt_t  *pUctxt
  )
{
  struct hal_frame *pFrame = UctxtFramePointer (pUctxt);

  switch ((UINTN) pFrame)
    {
    case 0:
      info ("INVALID/IDLE FRAME");
      break;
    default:
      hal_frame_print (pFrame);
    }
}

/**
  Bootstrap user context with entry point.

  Initializes user context with the user entry point from HAL.
  Returns FALSE if no user entry point is available.

  @param[in] pUctxt  User context to bootstrap.

  @retval TRUE   Context bootstrapped successfully.
  @retval FALSE  No user entry point available.
**/
BOOLEAN
UctxtBootstrap (
  IN uctxt_t  *pUctxt
  )
{
  vaddr_t UserEntry;

  UserEntry = hal_virtmem_userentry ();
  if (UserEntry == 0)
    {
      return FALSE;
    }

  UctxtInitialize (pUctxt, UserEntry, 0, 0);
  return TRUE;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use UctxtGet instead **/
uctxt_t *uctxt_get (struct hal_frame *f) {
  return UctxtGet (f);
}

/** @deprecated Use UctxtGetUser instead **/
uctxt_t *uctxt_getuser (struct hal_frame *f) {
  return UctxtGetUser (f);
}

/** @deprecated Use UctxtFramePointer instead **/
struct hal_frame *uctxt_frame_pointer (uctxt_t * uctxt) {
  return UctxtFramePointer (uctxt);
}

/** @deprecated Use UctxtFrame instead **/
struct hal_frame *uctxt_frame (uctxt_t * uctxt) {
  return UctxtFrame (uctxt);
}

/** @deprecated Use UctxtInitialize instead **/
void uctxt_init (uctxt_t * uctxt, vaddr_t ip, vaddr_t sp, vaddr_t gp) {
  UctxtInitialize (uctxt, ip, sp, gp);
}

/** @deprecated Use UctxtGetIp instead **/
vaddr_t uctxt_getip (uctxt_t * uctxt) {
  return UctxtGetIp (uctxt);
}

/** @deprecated Use UctxtSetIp instead **/
void uctxt_setip (uctxt_t * uctxt, vaddr_t ip) {
  UctxtSetIp (uctxt, ip);
}

/** @deprecated Use UctxtGetSp instead **/
vaddr_t uctxt_getsp (uctxt_t * uctxt) {
  return UctxtGetSp (uctxt);
}

/** @deprecated Use UctxtSetSp instead **/
void uctxt_setsp (uctxt_t * uctxt, vaddr_t sp) {
  UctxtSetSp (uctxt, sp);
}

/** @deprecated Use UctxtGetGp instead **/
vaddr_t uctxt_getgp (uctxt_t * uctxt) {
  return UctxtGetGp (uctxt);
}

/** @deprecated Use UctxtSetGp instead **/
void uctxt_setgp (uctxt_t * uctxt, vaddr_t gp) {
  UctxtSetGp (uctxt, gp);
}

/** @deprecated Use UctxtSetRet instead **/
void uctxt_setret (uctxt_t * uctxt, unsigned long ret) {
  UctxtSetRet (uctxt, ret);
}

/** @deprecated Use UctxtSetA0 instead **/
void uctxt_seta0 (uctxt_t * uctxt, unsigned long a0) {
  UctxtSetA0 (uctxt, a0);
}

/** @deprecated Use UctxtSetA1 instead **/
void uctxt_seta1 (uctxt_t * uctxt, unsigned long a1) {
  UctxtSetA1 (uctxt, a1);
}

/** @deprecated Use UctxtSetA2 instead **/
void uctxt_seta2 (uctxt_t * uctxt, unsigned long a2) {
  UctxtSetA2 (uctxt, a2);
}

/** @deprecated Use UctxtSetTls instead **/
void uctxt_settls (uctxt_t * uctxt, unsigned long tls) {
  UctxtSetTls (uctxt, tls);
}

/** @deprecated Use UctxtPrint instead **/
void uctxt_print (uctxt_t * uctxt) {
  UctxtPrint (uctxt);
}

/** @deprecated Use UctxtBootstrap instead **/
bool uctxt_bootstrap (uctxt_t * uctxt) {
  return UctxtBootstrap (uctxt);
}
