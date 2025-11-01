/** @file
  eCRT - An embedded C runtime library

  Enhanced printf implementation with comprehensive format modifier support

  Supports both MSVC and GCC/Clang format modifiers:
  - MSVC: I, I32, I64, w, hh, h
  - GCC/Clang: hh, h, l, ll, j, z, t, L

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

/*
 * Enhanced version of subr_prf.c with full MSVC and GCC/Clang modifier support
 * Based on NetBSD printf implementation
 */

#include <cdefs.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NBBY 8

typedef unsigned u_int;
typedef unsigned long u_long;
typedef long long longlong_t;
typedef unsigned long long u_longlong_t;

#define LIBSA_PRINTF_LONGLONG_SUPPORT 1
#define LIBSA_PRINTF_WIDTH_SUPPORT 1
#define LIBSA_PRINTF_PRECISION_SUPPORT 1

/* Size modifiers - bit flags */
#define MOD_CHAR         0x001  /* hh - char */
#define MOD_SHORT        0x002  /* h  - short */
#define MOD_LONG         0x004  /* l  - long */
#define MOD_LONGLONG     0x008  /* ll - long long */
#define MOD_INTMAX       0x010  /* j  - intmax_t */
#define MOD_SIZET        0x020  /* z  - size_t */
#define MOD_PTRDIFF      0x040  /* t  - ptrdiff_t */
#define MOD_I32          0x080  /* I32 - int32_t (MSVC) */
#define MOD_I64          0x100  /* I64 - int64_t (MSVC) */
#define MOD_W            0x200  /* w  - wchar_t (MSVC) */

/* Format flags */
#define FLG_ALT          0x0001  /* # - alternate form */
#define FLG_SPACE        0x0002  /* ' ' - space before positive */
#define FLG_LADJUST      0x0004  /* - - left adjust */
#define FLG_SIGN         0x0008  /* + - always show sign */
#define FLG_ZEROPAD      0x0010  /* 0 - zero pad */
#define FLG_NEGATIVE     0x0020  /* value is negative */
#define FLG_UPPERCASE    0x0040  /* uppercase hex */

static void kprintn(void (*)(int), unsigned long long, int, int, int);
static void sputchar(int);
static void kdoprnt(void (*)(int), const char *, va_list);

static char *sbuf, *ebuf;

const char hexdigits_lower[16] = "0123456789abcdef";
const char hexdigits_upper[16] = "0123456789ABCDEF";

static void
sputchar(int c)
{
    if (sbuf < ebuf)
        *sbuf++ = c;
}

int
vprintf(const char *fmt, va_list ap)
{
    kdoprnt(putchar, fmt, ap);
    return 0;
}

int
vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    sbuf = buf;
    ebuf = buf + size - 1;
    kdoprnt(sputchar, fmt, ap);
    *sbuf = '\0';
    return sbuf - buf;
}

static void
kdoprnt(void (*put)(int), const char *fmt, va_list ap)
{
    char *p;
    int ch;
    unsigned long long ul;
    int modifier;
    int flags;
    int width;
    int prec;
    const char *q;

    for (;;) {
        while ((ch = *fmt++) != '%') {
            if (ch == '\0')
                return;
            put(ch);
        }

        modifier = 0;
        flags = 0;
        width = 0;
        prec = -1;

reswitch:
        switch (ch = *fmt++) {
        /* Flags */
        case '#':
            flags |= FLG_ALT;
            goto reswitch;
        case ' ':
            flags |= FLG_SPACE;
            goto reswitch;
        case '-':
            flags |= FLG_LADJUST;
            goto reswitch;
        case '+':
            flags |= FLG_SIGN;
            goto reswitch;
        case '0':
            flags |= FLG_ZEROPAD;
            goto reswitch;

        /* Width */
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
            width = 0;
            do {
                width = width * 10 + (ch - '0');
                ch = *fmt;
                if ((unsigned)ch - '0' > 9)
                    break;
                ++fmt;
            } while (1);
            goto reswitch;

        case '*':
            width = va_arg(ap, int);
            if (width < 0) {
                flags |= FLG_LADJUST;
                width = -width;
            }
            goto reswitch;

        /* Precision */
        case '.':
            prec = 0;
            ch = *fmt;
            if (ch == '*') {
                ++fmt;
                prec = va_arg(ap, int);
                if (prec < 0)
                    prec = -1;
            } else {
                while ((unsigned)ch - '0' < 10) {
                    prec = prec * 10 + (ch - '0');
                    ch = *++fmt;
                }
            }
            goto reswitch;

        /* Size modifiers - GCC/Clang style */
        case 'h':
            if (*fmt == 'h') {
                ++fmt;
                modifier = MOD_CHAR;  /* hh */
            } else {
                modifier = MOD_SHORT;  /* h */
            }
            goto reswitch;

        case 'l':
            if (*fmt == 'l') {
                ++fmt;
                modifier = MOD_LONGLONG;  /* ll */
            } else {
                modifier = MOD_LONG;  /* l */
            }
            goto reswitch;

        case 'j':
            modifier = MOD_INTMAX;
            goto reswitch;

        case 'z':
            modifier = MOD_SIZET;
            goto reswitch;

        case 't':
            modifier = MOD_PTRDIFF;
            goto reswitch;

        /* Size modifiers - MSVC style */
        case 'I':
            if (*fmt == '3' && *(fmt+1) == '2') {
                fmt += 2;
                modifier = MOD_I32;  /* I32 */
            } else if (*fmt == '6' && *(fmt+1) == '4') {
                fmt += 2;
                modifier = MOD_I64;  /* I64 */
            } else {
                /* I alone means pointer-sized (size_t/ptrdiff_t) */
                modifier = (sizeof(void*) == 8) ? MOD_I64 : MOD_I32;
            }
            goto reswitch;

        case 'w':
            modifier = MOD_W;  /* wchar_t (MSVC) */
            goto reswitch;

        /* Conversions */
        case 'c':
            ch = va_arg(ap, int);
            width--;
            /* Left pad */
            if ((flags & FLG_LADJUST) == 0) {
                while (width-- > 0)
                    put(' ');
            }
            put(ch & 0xFF);
            /* Right pad */
            while (width-- > 0)
                put(' ');
            break;

        case 's':
            p = va_arg(ap, char *);
            if (p == NULL) {
                put('{');
                put('N');
                put('U');
                put('L');
                put('L');
                put('}');
                break;
            }

            /* Calculate string length for width */
            for (q = p; *q != '\0'; ++q)
                continue;
            if (prec >= 0 && (q - p) > prec)
                q = p + prec;

            width -= (q - p);

            /* Left pad */
            if ((flags & FLG_LADJUST) == 0) {
                while (width-- > 0)
                    put(' ');
            }

            /* Print string */
            while (p < q)
                put(*p++);

            /* Right pad */
            while (width-- > 0)
                put(' ');
            break;

        case 'd':
        case 'i':
            /* Get signed value based on modifier */
            if (modifier & (MOD_LONGLONG | MOD_I64 | MOD_INTMAX)) {
                ul = va_arg(ap, long long);
                if ((long long)ul < 0) {
                    ul = -(long long)ul;
                    flags |= FLG_NEGATIVE;
                }
            } else if (modifier & (MOD_LONG)) {
                ul = va_arg(ap, long);
                if ((long)ul < 0) {
                    ul = -(long)ul;
                    flags |= FLG_NEGATIVE;
                }
            } else if (modifier & MOD_SHORT) {
                ul = (short)va_arg(ap, int);
                if ((short)ul < 0) {
                    ul = -(short)ul;
                    flags |= FLG_NEGATIVE;
                }
            } else if (modifier & MOD_CHAR) {
                ul = (signed char)va_arg(ap, int);
                if ((signed char)ul < 0) {
                    ul = -(signed char)ul;
                    flags |= FLG_NEGATIVE;
                }
            } else {
                ul = va_arg(ap, int);
                if ((int)ul < 0) {
                    ul = -(int)ul;
                    flags |= FLG_NEGATIVE;
                }
            }
            kprintn(put, ul, 10, flags, width);
            break;

        case 'o':
        case 'u':
        case 'x':
        case 'X':
        case 'p':
            /* Handle pointer */
            if (ch == 'p') {
                modifier = (sizeof(void*) == 8) ? MOD_I64 : MOD_I32;
                flags |= FLG_ALT;
                ch = 'x';
            }

            if (ch == 'X')
                flags |= FLG_UPPERCASE;

            /* Get unsigned value based on modifier */
            if (modifier & (MOD_LONGLONG | MOD_I64 | MOD_INTMAX)) {
                ul = va_arg(ap, unsigned long long);
            } else if (modifier & (MOD_LONG | MOD_SIZET | MOD_PTRDIFF)) {
                ul = va_arg(ap, unsigned long);
            } else if (modifier & MOD_SHORT) {
                ul = (unsigned short)va_arg(ap, unsigned int);
            } else if (modifier & MOD_CHAR) {
                ul = (unsigned char)va_arg(ap, unsigned int);
            } else {
                ul = va_arg(ap, unsigned int);
            }

            if (ch == 'o')
                kprintn(put, ul, 8, flags, width);
            else if (ch == 'x' || ch == 'X')
                kprintn(put, ul, 16, flags, width);
            else  /* ch == 'u' */
                kprintn(put, ul, 10, flags, width);
            break;

        case '%':
            put('%');
            break;

        default:
            if (ch == '\0')
                return;
            put('%');
            if (ch != '\0')
                put(ch);
            break;
        }
    }
}

static void
kprintn(void (*put)(int), unsigned long long ul, int base, int flags, int width)
{
    char buf[(sizeof(unsigned long long) * NBBY / 3) + 1 + 3];  /* ALT + SIGN + SPACE */
    char *p = buf;
    const char *hexdigits = (flags & FLG_UPPERCASE) ? hexdigits_upper : hexdigits_lower;

    /* Convert number to string (reversed) */
    do {
        *p++ = hexdigits[ul % base];
    } while (ul /= base);

    /* Add prefix if needed */
    if (flags & FLG_ALT) {
        if (base == 8 && *(p - 1) != '0') {
            *p++ = '0';
        } else if (base == 16) {
            *p++ = (flags & FLG_UPPERCASE) ? 'X' : 'x';
            *p++ = '0';
        }
    }

    /* Add sign */
    if (flags & FLG_NEGATIVE)
        *p++ = '-';
    else if (flags & FLG_SIGN)
        *p++ = '+';
    else if (flags & FLG_SPACE)
        *p++ = ' ';

    width -= (p - buf);

    /* Padding */
    if ((flags & (FLG_ZEROPAD | FLG_LADJUST)) == 0) {
        /* Left pad with spaces */
        while (width-- > 0)
            put(' ');
    }

    /* Output prefix/sign */
    char *endp = p;
    while (p > buf && (buf[p-1-buf] == '-' || buf[p-1-buf] == '+' || buf[p-1-buf] == ' ' ||
                       buf[p-1-buf] == '0' || buf[p-1-buf] == 'x' || buf[p-1-buf] == 'X')) {
        put(*--p);
    }

    /* Zero pad if needed */
    if (flags & FLG_ZEROPAD) {
        while (width-- > 0)
            put('0');
    }

    /* Output digits */
    while (p > buf)
        put(*--p);

    /* Right pad */
    if (flags & FLG_LADJUST) {
        while (width-- > 0)
            put(' ');
    }
}
