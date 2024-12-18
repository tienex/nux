#include <stdio.h>
#include "internal.h"
#include <string.h>

void
hal_frame_init (struct hal_frame *f)
{
  memset (f, 0, sizeof (*f));
  f->sstatus = SSTATUS_SPIE;
  f->sie = SIE_USER;
}

bool
hal_frame_isuser (struct hal_frame *f)
{
  return !(f->sstatus & SSTATUS_SPP);
}

void
hal_frame_setip (struct hal_frame *f, unsigned long ip)
{
  f->pc = ip;
}

void
hal_frame_setsp (struct hal_frame *f, unsigned long sp)
{
  f->sp = sp;
}

void
hal_frame_setgp (struct hal_frame *f, unsigned long gp)
{
  f->gp = gp;
}

vaddr_t
hal_frame_getgp (struct hal_frame *f)
{
  return f->gp;
}

unsigned long
hal_frame_getip (struct hal_frame *f)
{
  return f->pc;
}

unsigned long
hal_frame_getsp (struct hal_frame *f)
{
  return f->sp;
}

void
hal_frame_seta0 (struct hal_frame *f, unsigned long a0)
{
  f->a0 = a0;
}

void
hal_frame_seta1 (struct hal_frame *f, unsigned long a1)
{
  f->a1 = a1;
}

void
hal_frame_seta2 (struct hal_frame *f, unsigned long a2)
{
  f->a2 = a2;
}

void
hal_frame_setret (struct hal_frame *f, unsigned long ret)
{
  f->a0 = ret;
}

void
hal_frame_settls (struct hal_frame *f, unsigned long ret)
{
  f->tp = ret;
}

void
hal_frame_print (struct hal_frame *f)
{
  info ("   PC: %016lx   SP: %016lx    SIE: %016lx", f->pc, f->sp, f->sie);
  info ("STVAL:%016lx CAUSE: %016lx STATUS: %016lx",
	f->stval, f->scause, f->sstatus);
  info ("   RA: %016lx   GP: %016lx     TP: %016lx", f->ra, f->gp, f->tp);
  info ("   T0: %016lx   T1: %016lx     T2: %016lx", f->t0, f->t1, f->t2);
  info ("   T3: %016lx   T4: %016lx     T5: %016lx", f->t3, f->t4, f->t5);
  info ("   T6: %016lx   FP: %016lx     S1: %016lx", f->t6, f->fp, f->s1);
  info ("   S2: %016lx   S3: %016lx     S4: %016lx", f->s2, f->s3, f->s4);
  info ("   S5: %016lx   S6: %016lx     S7: %016lx", f->s5, f->s6, f->s7);
  info ("   S8: %016lx   S9: %016lx    S10: %016lx", f->s8, f->s9, f->s10);
  info ("  S11: %016lx   A0: %016lx     A1: %016lx", f->s11, f->a0, f->a1);
  info ("   A2: %016lx   A3: %016lx     A4: %016lx", f->a2, f->a3, f->a4);
  info ("   A5: %016lx   A6: %016lx     A7: %016lx", f->a5, f->a6, f->a7);
}
