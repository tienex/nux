typedef unsigned long u_long;
/* *INDENT-OFF* */ /* Imported from NetBSD -- MHDIFFIGNORE */
/* $NetBSD: crt0-common.c,v 1.13 2013/01/31 22:24:25 matt Exp $ */

/*
 * Copyright (c) 1998 Christos Zoulas
 * Copyright (c) 1995 Christopher G. Demetriou
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#ifndef _EC_SOURCE
#include <sys/cdefs.h>
__RCSID("$NetBSD: crt0-common.c,v 1.13 2013/01/31 22:24:25 matt Exp $");

#include <sys/types.h>
#include <sys/exec.h>
#include <sys/syscall.h>
#include <machine/profile.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtld.h"

#else /* _EC_SOURCE */

#include <cdefs.h>
#include <stdlib.h>
#include <machine/profile.h>

#define Obj_Entry void
typedef void (*fptr_t)(void);

#endif

extern int main(int, char **);

#ifndef HAVE_INITFINI_ARRAY
extern void	_init(void);
extern void	_fini(void);
#endif

#ifndef _EC_SOURCE
extern void	_libc_init(void);
#endif

#ifdef MCRT0
extern void	monstartup(u_long, u_long);
extern void	_mcleanup(void);
extern unsigned char __etext, __eprol;
#endif /* MCRT0 */

char		**environ;
struct ps_strings *__ps_strings = 0;

static char	 empty_string[] = "";
char		*__progname = empty_string;

__dead void ___start(char *cmdline);

#ifdef _EC_SOURCE
#define write(fd, s, n)
#else
#define	write(fd, s, n)	__syscall(SYS_write, (fd), (s), (n))
#endif

#define	_FATAL(str)				\
do {						\
	write(2, str, sizeof(str)-1);		\
	_exit(1);				\
} while (0)

#ifdef HAVE_INITFINI_ARRAY
/*
 * If we are using INIT_ARRAY/FINI_ARRAY and we are linked statically,
 * we have to process these instead of relying on RTLD to do it for us.
 *
 * Since we don't need .init or .fini sections, just code them in C
 * to make life easier.
 */
__weakref_visible const fptr_t preinit_array_start[1]
    __weak_reference(__preinit_array_start);
__weakref_visible const fptr_t preinit_array_end[1]
    __weak_reference(__preinit_array_end);
__weakref_visible const fptr_t init_array_start[1]
    __weak_reference(__init_array_start);
__weakref_visible const fptr_t init_array_end[1]
    __weak_reference(__init_array_end);
__weakref_visible const fptr_t fini_array_start[1]
    __weak_reference(__fini_array_start);
__weakref_visible const fptr_t fini_array_end[1]
    __weak_reference(__fini_array_end);

static inline void
_preinit(void)
{
	const fptr_t *f;
	for (f = preinit_array_start; f < preinit_array_end; f++) {
		(*f)();
	}
}

static inline void
_init(void)
{
	const fptr_t *f;
	for (f = init_array_start; f < init_array_end; f++) {
		(*f)();
	}
}

static void
_fini(void)
{
	const fptr_t *f;
	for (f = fini_array_start; f < fini_array_end; f++) {
		(*f)();
	}
}
#endif /* HAVE_INITFINI_ARRAY */

static unsigned _argc = 0;
static char *_argv[16];

static char *
strsep(char **stringp, const char *delim)
{
	char *s;
	const char *spanp;
	int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

void
___start(char *cmdline)
{

	if (cmdline != NULL) {
		char **ap;

		for (ap = _argv; (*ap = strsep(&cmdline, " \t")) != NULL;) {
			if (**ap != '\0') {
				if (++ap >= &_argv[16])
					break;
				_argc++;
			}
		}
	}

	if (_argv[0] != NULL) {
		char *c;
		__progname = _argv[0];
		for (c = _argv[0]; *c; ++c) {
			if (*c == '/')
				__progname = c + 1;
		}
	}

#ifdef HAVE_INITFINI_ARRAY
	_preinit();
#endif

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&__eprol, (u_long)&__etext);
#endif

	atexit(_fini);
	_init();

	exit(main(_argc, _argv));
}
