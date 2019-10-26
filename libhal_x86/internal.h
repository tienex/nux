#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <nux/hal_config.h>

#define MAXCPUS	        HAL_MAXCPUS

#define MSR_IA32_EFER 0xc0000080
#define _MSR_IA32_EFER_NXE (1LL << 11)

#ifndef _ASSEMBLER

#include <nux/nux.h>

#define haldebug(...) debug(__VA_ARGS__)
#define hallog(...) info(__VA_ARGS__)
#define halwarn(...) warn(__VA_ARGS__)
#define halfatal(...) fatal(__VA_ARGS__)

void x86_init (void);
void pmap_init (void);

int vga_putchar (int c);

uint64_t rdmsr (uint32_t ecx);

#endif

#endif
