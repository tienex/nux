#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <nux/hal_config.h>

#define MAXCPUS	        HAL_MAXCPUS

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_NXE (1LL << 11)

#ifndef _ASSEMBLER

#include <stdint.h>

#define haldebug(...) hal_srv_debug (__FILE__, __LINE__, __VA_ARGS__)
#define hallog(...) hal_srv_info (__FILE__, __LINE__, __VA_ARGS__)
#define halwarn(...) hal_srv_warn (__FILE__, __LINE__, __VA_ARGS__)
#define halfatal(...) hal_srv_fatal (__FILE__, __LINE__, __VA_ARGS__)

void x86_init (void);
void pmap_init (void);

int vga_putchar (int c);

uint64_t rdmsr (uint32_t ecx);

#endif

#endif
