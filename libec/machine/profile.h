#ifndef EC_MACHINE_PROFILE_H
#define EC_MACHINE_PROFILE_H

#if defined(__riscv) && __riscv_xlen == 64
#define HAVE_INITFINI_ARRAY
#endif

#endif
