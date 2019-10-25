#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <nux/hal_config.h>

#define MAXCPUS	        HAL_MAXCPUS

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_NXE (1LL << 11)

#ifndef _ASSEMBLER

#include <stdint.h>

#define haldebug(...) printf("D:"__VA_ARGS__)
#define hallog(...) printf(__VA_ARGS__)
#define halwarn(...) printf("WARN:" __VA_ARGS__)
#define halfatal(...) assert (0 && __VA_ARGS__)

void x86_init (void);
void pmap_init (void);

int vga_putchar (int c);

uint64_t rdmsr (uint32_t ecx);

#endif

#endif
