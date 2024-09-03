#ifndef _HAL_INTERNAL_H
#define _HAL_INTERNAL_H

#include <nux/hal_config.h>

unsigned long umap_minaddr (void);
unsigned long umap_maxaddr (void);


#define GET_FAR_POINTER(_x)			\
  ({						\
    uintptr_t ptr;				\
    asm volatile (				\
		  "nop\n"			\
		  ".option push\n"		\
		  ".option norelax\n"		\
		  "la %0, " #_x "\n"			\
		  ".option pop"			\
		  : "=r" (ptr)			\
		  : "i" (_x));			\
    ptr;					\
  })

#endif /* _HAL_INTERNAL_H */
