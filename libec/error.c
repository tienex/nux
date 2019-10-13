#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void
info (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);

  printf ("\n");
}

static void
do_err (bool do_exit, const char *mode, const char *fmt, va_list ap)
{
  printf ("%s:", mode);
  vprintf (fmt, ap);
  printf ("\n");

  if (do_exit)
    {
      exit (-1);
    }
}

void
alert (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  do_err (false, "ALERT", fmt, ap);
  va_end (ap);
}

void
error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  do_err (false, "ERROR", fmt, ap);
  va_end (ap);
}

void
fatal (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  do_err (true, "FATAL", fmt, ap);
  va_end (ap);

  /* Not Reached Anyway. */
  exit (-1);
}
