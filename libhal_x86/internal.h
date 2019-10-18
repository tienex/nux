#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <nux/hal_config.h>

#define MAXCPUS	        HAL_MAXCPUS

#ifndef _ASSEMBLER

#define haldebug(...) hal_srv_debug (__FILE__, __LINE__, __VA_ARGS__)
#define hallog(...) hal_srv_info (__FILE__, __LINE__, __VA_ARGS__)
#define halwarn(...) hal_srv_warn (__FILE__, __LINE__, __VA_ARGS__)
#define halfatal(...) hal_srv_fatal (__FILE__, __LINE__, __VA_ARGS__)

void x86_init (void);

#endif

#endif
