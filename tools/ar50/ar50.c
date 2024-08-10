#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "squoze.h"

#define PROGNAME "ar50"
#define VERSION "0.0"

/*
  AR50 - A simple archive format.

  AR50 is a simple archive format, meant for embedded parsing. It is a
  collection of header and file sequences. The header is fixed
  sized. The filename is encoded in a 64-bit DEC RAD-50 string. This
  limits both the length and the characters.
*/

#define PAYLOAD_HDR_MAGIC 0x68efe6966e3e3bb5LL	/* RAD-50 for 'rad50archive' */

uint64_t magic = 0x68efe6966e3e3bb5LL;

/*
  On disk structure with payload information.
*/
struct payload_hdr
{

  uint64_t magic;
  uint64_t filename;
  uint32_t size;
} __attribute__((packed));


void
report (const char *format, va_list args)
{
  fflush (stdout);
  fprintf (stderr, PROGNAME ": ");
  vfprintf (stderr, format, args);
  putc ('\n', stderr);
}

void
non_fatal (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  report (format, args);
  va_end (args);
}

void
fatal (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  report (format, args);
  va_end (args);
  exit (-1);
}


static void
usage (FILE * f, int status)
{
  fprintf (f, "Usage: %s [command]\n", PROGNAME);
  fprintf (f, " NUX archive utility.\n");
  fprintf (f, " Command is one of the following:\n\
  %s [options] {-l|--list} <archive>              List all files in archive\n\
  %s [options] {-c|--create} <archive> <files>... Create a new archive containing <files>.\n\
  %s [options] {-x|--extract} <archive>           Extract all files in current directory.\n\
  %s {-V|--version}                               Display this program's version number\n\
  %s {-h|--help}                                  Display this information\n\
\n\
 Options:\n\
  -m <string>		Use 'string' as magic value.\n\
\n", PROGNAME, PROGNAME, PROGNAME, PROGNAME, PROGNAME);
  exit (status);
}

static void
print_version (void)
{
  printf ("%s %s\n", PROGNAME, VERSION);
  printf ("Copyright (C) 2015-2023 Gianluca Guida.\n");
  printf ("\
This program is free software; you may redistribute it under the terms of\n\
the GNU General Public License version 3 or (at your option) any later version.\n\
This program has absolutely no warranty.\n");
  exit (0);
}

void
do_list (char *filename)
{
  FILE *f;
  struct payload_hdr hdr;

  f = fopen (filename, "r");
  if (f == NULL)
    {
      fatal ("%s:%s", filename, strerror (errno));
    }

  while (!(fread ((void *) &hdr, 1, sizeof (hdr), f) == 0 || ferror (f)))
    {
      if (hdr.magic != magic)
	fatal ("Corrupted entry (Bad Magic)");
      char *name = unsquoze (hdr.filename);
      fprintf (stdout, "%12s: %-10u %08lx\n", name, hdr.size, ftell (f));
      free (name);
      fseek (f, hdr.size, SEEK_CUR);
    }

  if (!feof (f))
    fatal ("Cannot read archive: %s", strerror (errno));
}

void
do_create (char *filename, char *const list[])
{
  int r;
  FILE *f, *out;
  char *n;
  void *buf;
  size_t size;
  struct stat st;
  struct payload_hdr *hdr;

  out = fopen (filename, "w");
  if (out == NULL)
    {
      fatal ("%s:%s", filename, strerror (errno));
    }

  while ((n = *list++))
    {
      f = fopen (n, "r");
      if (f == NULL)
	fatal ("%s: %s", n, strerror (errno));

      r = stat (n, &st);
      if (r < 0)
	fatal ("%s: stat failed: %s", n, strerror (errno));

      size = sizeof (struct payload_hdr) + st.st_size;

      buf = calloc (1, size);
      if (buf == NULL)
	fatal ("calloc failed");

      hdr = (struct payload_hdr *) buf;
      hdr->magic = magic;
      hdr->filename = squoze (n);
      hdr->size = st.st_size;

      if (fread ((void *) (hdr + 1), 1, st.st_size, f) == 0 || ferror (f))
	fatal ("%s: fread failed", n);
      fclose (f);

      if ((fwrite (buf, 1, size, out) == 0) || ferror (out))
	fatal ("Can't write to output file: %s", strerror (errno));
    }
  fclose (out);
  exit (0);
}

void
do_extract (char *filename)
{
  FILE *f;
  struct payload_hdr hdr;

  f = fopen (filename, "r");
  if (f == NULL)
    {
      fatal ("%s:%s", filename, strerror (errno));
    }

  while (!(fread ((void *) &hdr, 1, sizeof (hdr), f) == 0 || ferror (f)))
    {
      FILE *out;
      void *buf;

      if (hdr.magic != magic)
	fatal ("Corrupted entry (Bad Magic)");
      char *name = unsquoze (hdr.filename);
      buf = calloc (1, hdr.size);
      if (buf == NULL)
	fatal ("calloc failed");
      out = fopen (name, "w");
      if (f == NULL)
	{
	  fatal ("%s:%s", name, strerror (errno));
	}
      if (fread (buf, 1, hdr.size, f) == 0 || ferror (f))
	fatal ("%s: fread failed", name);
      if ((fwrite (buf, 1, hdr.size, out) == 0) || ferror (out))
	fatal ("Can't write to output file %s: %s", name, strerror (errno));
      fclose (out);
      free (buf);
      free (name);
    }

  if (!feof (f))
    fatal ("Cannot read archive: %s", strerror (errno));
}


const struct option long_options[] = {
  {"list", no_argument, NULL, 'l'},
  {"create", no_argument, NULL, 'c'},
  {"extract", no_argument, NULL, 'x'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  {0, no_argument, 0, 0}
};

int
main (int argc, char *const argv[])
{
  bool create, extract, list, show_version;
  unsigned cmdseen;
  char *filename;
  char c;

  cmdseen = 0;
  create = list = show_version = false;

  while ((c = getopt_long (argc, argv, "cxlhVm:", long_options, NULL)) != EOF)
    switch (c)
      {
      case 'c':
	cmdseen++;
	create = true;
	break;
      case 'x':
	cmdseen++;
	extract = true;
	break;
      case 'l':
	cmdseen++;
	list = true;
	break;
      case 'V':
	cmdseen++;
	show_version = true;
	break;
      case 'h':
	usage (stdout, 0);
	break;
      case 'm':
	magic = squoze (optarg);
	break;
      default:
	usage (stderr, 1);
      }

  argc -= optind;
  argv += optind;

  if (cmdseen != 1)
    usage (stderr, 1);
  if (show_version)
    {
      if (argc != 0)
	{
	  usage (stderr, 1);
	}
      print_version ();
    }
  if (argc < 1)
    usage (stderr, 1);
  filename = argv[0];
  argc -= 1;
  argv += 1;

  if (create)
    {
      if (argc == 0)
	{
	  usage (stderr, 1);
	}
      do_create (filename, argv);
    }

  if (extract)
    {
      if (argc != 0)
	{
	  usage (stderr, 1);
	}
      do_extract (filename);
    }

  if (list)
    {
      if (argc != 0)
	{
	  usage (stderr, 1);
	}
      do_list (filename);
    }

}
