#ifndef _NUX_NMIEMUL_H
#define _NUX_NMIEMUL_H

#include <nux/hal.h>

/*
  Some architecture do not have the ability to support NUX model: a NMI to
  interrupt the kernel and an IPI to interrupt userspace.
 
  This library emulates this.
 */

struct hal_frame *nmiemul_entry (struct hal_frame *f);
struct hal_frame *nmiemul_ipicheck (struct hal_frame *f);

void nmiemul_nmi_set (unsigned cpu);
void nmiemul_nmi_setall (void);

bool nmiemul_ipi_pending (void);
void nmiemul_ipi_clear (void);
void nmiemul_ipi_set (unsigned cpu);
void nmiemul_ipi_setall (void);

#endif
