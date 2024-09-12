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

#define SSTATUS_FS_OFF   (0 << 13)
#define SSTATUS_FS_INIT  (1 << 13)
#define SSTATUS_FS_CLEAN (2 << 13)
#define SSTATUS_FS_DIRTY (3 << 13)

#define SSTATUS_VS_OFF   (0 << 9)
#define SSTATUS_VS_INIT  (1 << 9)
#define SSTATUS_VS_CLEAN (2 << 9)
#define SSTATUS_VS_DIRTY (3 << 9)

#define SSTATUS_XS_OFF   (0 << 15)
#define SSTATUS_XS_INIT  (1 << 15)
#define SSTATUS_XS_CLEAN (2 << 15)
#define SSTATUS_XS_DIRTY (3 << 15)

#define SCAUSE_INTR (1L << 63)
#define SCAUSE_SSI  (SCAUSE_INTR|1)
#define SCAUSE_STI  (SCAUSE_INTR|5)
#define SCAUSE_SEI  (SCAUSE_INTR|9)

#define SCAUSE_SYSC  8
#define SCAUSE_IPF  12
#define SCAUSE_LPF  13
#define SCAUSE_SPF  15

#define SIE_SSIE (1L << 1)
#define SIE_STIE (1L << 5)
#define SIE_SEIE (1L << 9)
#define SIE_LOCFIE (1L << 13)

#define SIP_SSIP SIE_SSIE

#endif
