/*	$NetBSD: strncmp.c,v 1.3 2018/02/04 20:22:17 mrg Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _EC_SOURCE

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)strncmp.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID ("$NetBSD: strncmp.c,v 1.3 2018/02/04 20:22:17 mrg Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <assert.h>
#include <string.h>
#else
#include <lib/libkern/libkern.h>
#endif

#else /* _EC_SOURCE */

#define _DIAGASSERT(_x)
#include <cdefs.h>
#include <assert.h>
#include <string.h>

#endif /* _EC_SOURCE */

int
strncmp (const char *s1, const char *s2, size_t n)
{

  if (n == 0)
    return (0);
  do
    {
      if (*s1 != *s2++)
	return (*(const unsigned char *) s1 - *(const unsigned char *) --s2);
      if (*s1++ == 0)
	break;
    }
  while (--n != 0);
  return (0);
}
