#ifndef EC_STDLIB_H
#define EC_STDLIB_H

#include <cdefs.h>

int atexit (void (*func) (void));
void __dead exit (int status);	/* EXTERNAL */

#endif
