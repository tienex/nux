/**INDENT-OFF**/
/* Imported from NetBSD -- MHDIFFIGNORE */
/*	$NetBSD: snprintf.c,v 1.5 2011/07/17 20:54:52 joerg Exp $	*/

/*-
 * Copyright (c) 1993
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
 *
 *	@(#)printf.c	8.1 (Berkeley) 6/11/93
 */

#if defined(_EC_SOURCE)
#include <cdefs.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#else /* _EC_SOURCE */
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/stdarg.h>

#if defined(_LIBC)
#include <stdio.h>

#include "namespace.h"

#ifdef __weak_alias
__weak_alias (vsnprintf, _vsnprintf)
__weak_alias (vsnprintf_l, _vsnprintf_l)
__weak_alias (snprintf, _snprintf) __weak_alias (snprintf_l, _snprintf_l)
#endif
#else
#include "stand.h"
#endif
#endif /* __MURGIA__ */


int
snprintf (char *buf, size_t size, const char *fmt, ...)
{
  va_list ap;
  int len;

  va_start (ap, fmt);
  len = vsnprintf (buf, size, fmt, ap);
  va_end (ap);
  return len;
}
