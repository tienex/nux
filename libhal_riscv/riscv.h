#ifndef HAL_RISCV_RISCV_H
#define HAL_RISCV_RISCV_H

#define SSTATUS_SIE  (1L << 1)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_UBE  (1L << 6)
#define SSTATUS_SPP  (1L << 8)
#define SSTATUS_VS   (1L << 9)
#define SSTATUS_FS   (1L << 13)
#define SSTATUS_XS   (1L << 15)
#define SSTATUS_SUM  (1L << 18)
#define SSTATUS_MXR  (1L << 19)
#define SSTATUS_UXL  (1L << 32)
#define SSTATUS_SD   (1L << 63)

#endif
