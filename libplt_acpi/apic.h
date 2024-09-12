#ifndef PLTACPI_APIC_H
#define PLTACPI_APIC_H

/*
  This is the main apic gsi/ipi to interrupt vector configuration.

  Based on this parameters, the HAL will receive interrupt at vectors
  in this range. The HAL does not need to understand the meaning of
  the vectors, the platform library will translate that.
*/

#define APIC_VECT_MAX     hal_vect_max()
#define APIC_VECT_IPIMAX  1
#define APIC_VECT_IPIBASE (APIC_VECT_MAX - APIC_VECT_IPIMAX)
#define APIC_VECT_IRQBASE 0x28
#define APIC_VECT_IRQMAX (APIC_VECT_IPIBASE - APIC_VECT_IRQBASE)

#endif
