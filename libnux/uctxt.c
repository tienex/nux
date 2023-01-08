/*
 * NUX: A kernel Library. Copyright (C) 2019 Gianluca Guida,
 * glguida@tlbflush.org
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * See COPYING file for the full license.
 * 
 * SPDX-License-Identifier:	GPL2.0+
 */


#include <assert.h>
#include <nux/nux.h>

#include "internal.h"

uctxt_t *
uctxt_get (struct hal_frame *f)
{
  bool wasidle = cpu_wasidle ();

  if (hal_frame_isuser (f))
    {
      assert (!wasidle);
      return (uctxt_t *) f;
    }
  else if (wasidle)
    {
      cpu_clridle ();
      return UCTXT_IDLE;
    }
  else
    {
      return UCTXT_INVALID;
    }
}

uctxt_t *
uctxt_getuser (struct hal_frame *f)
{
  uctxt_t *uctxt;

  uctxt = uctxt_get (f);

  if (uctxt == UCTXT_INVALID)
    {
      fatal ("Expected User Frame.");
    }
  return uctxt;
}

struct hal_frame *
uctxt_frame_pointer (uctxt_t * uctxt)
{
  if (uctxt != UCTXT_INVALID && uctxt != UCTXT_IDLE)
    return (struct hal_frame *) uctxt;

  return NULL;
}

struct hal_frame *
uctxt_frame (uctxt_t * uctxt)
{
  assert (uctxt != UCTXT_INVALID);

  if (uctxt == UCTXT_IDLE)
    {
      cpu_idle ();
    }
  else
    {
      return (struct hal_frame *) uctxt;
    }
}

void
uctxt_init (uctxt_t * uctxt, vaddr_t ip, vaddr_t sp)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);
  assert (f);

  hal_frame_init (f);
  hal_frame_setip (f, ip);
  hal_frame_setsp (f, sp);
}

void
uctxt_setip (uctxt_t * uctxt, vaddr_t ip)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);
  assert (f);
  hal_frame_setip (f, ip);
}

void
uctxt_setsp (uctxt_t * uctxt, vaddr_t sp)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);
  assert (f);
  hal_frame_setsp (f, sp);
}

void
uctxt_setret (uctxt_t * uctxt, unsigned long ret)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);
  assert (f);
  hal_frame_setret (f, ret);
}

void
uctxt_seta0 (uctxt_t * uctxt, unsigned long a0)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);
  assert (f);
  hal_frame_seta0 (f, a0);
}

void
uctxt_seta1 (uctxt_t * uctxt, unsigned long a1)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);
  assert (f);
  hal_frame_seta1 (f, a1);
}

void
uctxt_seta2 (uctxt_t * uctxt, unsigned long a2)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);
  assert (f);
  hal_frame_seta1 (f, a2);
}

void
uctxt_print (uctxt_t * uctxt)
{
  struct hal_frame *f = uctxt_frame_pointer (uctxt);

  switch ((uintptr_t) f)
    {
    case 0:
      info ("INVALID/IDLE FRAME");
      break;
    default:
      hal_frame_print (f);
    }
}

bool
uctxt_bootstrap (uctxt_t * uctxt)
{
  vaddr_t uentry;

  uentry = hal_virtmem_userentry ();
  if (uentry == 0)
    {
      return false;
    }
  uctxt_init (uctxt, uentry, 0);
  return true;
}
