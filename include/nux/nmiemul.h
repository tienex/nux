#ifndef _NUX_NMIEMUL_H
#define _NUX_NMIEMUL_H

/*
  Some architecture do not have the ability to support NUX model: a
  NMI to interrupt the kernel and an IPI to interrupt userspace.

  This library emulates this.
*/

bool nmiemul_nmi_pending (void);
void nmiemul_nmi_clear (void);
void nmiemul_nmi_set (unsigned cpu);
void nmiemul_nmi_setall (void);

bool nmiemul_ipi_pending (void);
void nmiemul_ipi_clear (void);
void nmiemul_ipi_set (unsigned cpu);
void nmiemul_ipi_setall (void);

#endif
