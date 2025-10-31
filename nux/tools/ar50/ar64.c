/** @file
  AR64 Archive Utility

  Advanced archive format using Zoo64 fixed-point range arithmetic encoding
  for filenames. AR64 archives store files with fixed-size headers containing
  magic number, Zoo64 encoded filename (up to 16 chars with full ASCII support),
  and file size. Provides better compression and character support than AR50.

  Key improvements over AR50:
  - Supports up to 16 characters (vs 12 in AR50)
  - Full printable ASCII character set (vs limited RAD-50 charset)
  - Adaptive fixed-point range arithmetic encoding
  - Zoo format expanded to 64-bit

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "zoo64.h"
#include "compress.h"

#define PROGNAME "ar64"
#define VERSION "2.0"

/*
  AR64 - Advanced archive format with Zoo64 encoding.

  AR64 is an advanced archive format using fixed-point range arithmetic
  for adaptive filename encoding. It is a collection of header and file
  sequences with fixed-sized headers. The filename is encoded in a 64-bit
  Zoo64 string using adaptive range coding with fixed-point arithmetic.
  This provides better compression and supports more characters than RAD-50.
*/

#define PAYLOAD_HDR_MAGIC 0x9a7c6e5f4d3b2a19LL	/* Zoo64 for 'ar64zoo' */

UINT64 gMagic = 0x9a7c6e5f4d3b2a19LL;

/*
  On disk structure with payload information.

  New format with compression support:
  - flags: bit 0 = compressed, bits 1-7 reserved
  - size: compressed size (if compressed) or original size
  - orig_size: original size (only if compressed)
*/
typedef struct _PAYLOAD_HDR
{
  UINT64 magic;
  UINT64 filename;
  UINT8  flags;        // Compression flags
  UINT32 size;         // Compressed/stored size
  UINT32 orig_size;    // Original size (if compressed)
} __attribute__((packed)) PAYLOAD_HDR;

#define FLAG_COMPRESSED 0x01


/**
  Report error message.

  Internal helper for formatting error messages to stderr.

  @param[in] pFormat  Printf-style format string.
  @param[in] Args     Variable argument list.
**/
VOID
Report (
  IN CONST CHAR8  *pFormat,
  IN va_list      Args
  )
{
  fflush (stdout);
  fprintf (stderr, PROGNAME ": ");
  vfprintf (stderr, pFormat, Args);
  putc ('\n', stderr);
}

/**
  Report non-fatal error.

  Prints error message to stderr but continues execution.

  @param[in] pFormat  Printf-style format string.
  @param[in] ...      Variable arguments.
**/
VOID
NonFatal (
  IN CONST CHAR8  *pFormat,
  ...
  )
{
  va_list Args;

  va_start (Args, pFormat);
  Report (pFormat, Args);
  va_end (Args);
}

/**
  Report fatal error and exit.

  Prints error message to stderr and terminates program with exit code -1.

  @param[in] pFormat  Printf-style format string.
  @param[in] ...      Variable arguments.
**/
VOID
Fatal (
  IN CONST CHAR8  *pFormat,
  ...
  )
{
  va_list Args;

  va_start (Args, pFormat);
  Report (pFormat, Args);
  va_end (Args);
  exit (-1);
}


/**
  Print usage information.

  Displays command-line usage and available options.

  @param[in] pF      Output file stream (stdout or stderr).
  @param[in] Status  Exit status code.
**/
static VOID
Usage (
  IN FILE  *pF,
  IN int   Status
  )
{
  fprintf (pF, "Usage: %s [command]\n", PROGNAME);
  fprintf (pF, " NUX AR64 archive utility (Zoo64 format with fixed-point range arithmetic).\n");
  fprintf (pF, " Command is one of the following:\n\
  %s [options] {-l|--list} <archive>              List all files in archive\n\
  %s [options] {-c|--create} <archive> <files>... Create a new archive containing <files>.\n\
  %s [options] {-x|--extract} <archive>           Extract all files in current directory.\n\
  %s {-V|--version}                               Display this program's version number\n\
  %s {-h|--help}                                  Display this information\n\
\n\
 Options:\n\
  -m <string>		Use 'string' as magic value.\n\
\n\
 Features:\n\
  - Zoo64 format: Fixed-point range arithmetic adaptive encoding\n\
  - Supports up to 16 character filenames (vs 12 in AR50)\n\
  - Full printable ASCII support (space through tilde)\n\
  - Better compression than RAD-50 encoding\n\
\n", PROGNAME, PROGNAME, PROGNAME, PROGNAME, PROGNAME);
  exit (Status);
}

/**
  Print version information.

  Displays program name, version, and copyright.
**/
static VOID
PrintVersion (
  VOID
  )
{
  printf ("%s %s\n", PROGNAME, VERSION);
  printf ("Copyright (C) 2015-2025 Gianluca Guida.\n");
  printf ("Zoo64 format: Base-96 positional encoding with fixed-point arithmetic\n");
  exit (0);
}

/**
  List archive contents.

  Reads archive and displays filename, size, and file offset for each entry.

  @param[in] pFilename  Archive filename.
**/
VOID
DoList (
  IN CHAR8  *pFilename
  )
{
  FILE *pF;
  PAYLOAD_HDR Hdr;

  pF = fopen (pFilename, "r");
  if (pF == NULL)
    {
      Fatal ("%s:%s", pFilename, strerror (errno));
    }

  while (!(fread ((void *) &Hdr, 1, sizeof (Hdr), pF) == 0 || ferror (pF)))
    {
      if (Hdr.magic != gMagic)
	Fatal ("Corrupted entry (Bad Magic)");
      char *pName = Zoo64Decode (Hdr.filename);
      if (pName == NULL)
	Fatal ("Failed to decode filename");
      fprintf (stdout, "%16s: %-10u %08lx\n", pName, Hdr.size, ftell (pF));
      free (pName);
      fseek (pF, Hdr.size, SEEK_CUR);
    }

  if (!feof (pF))
    Fatal ("Cannot read archive: %s", strerror (errno));
}

/**
  Create new archive.

  Creates archive containing specified files with Zoo64 encoded filenames.

  @param[in] pFilename  Archive filename to create.
  @param[in] ppList     NULL-terminated array of filenames to archive.
**/
VOID
DoCreate (
  IN CHAR8         *pFilename,
  IN CHAR8 *CONST  ppList[]
  )
{
  int R;
  FILE *pF, *pOut;
  CHAR8 *pN;
  VOID *pBuf;
  size_t Size;
  struct stat St;
  PAYLOAD_HDR *pHdr;

  pOut = fopen (pFilename, "w");
  if (pOut == NULL)
    {
      Fatal ("%s:%s", pFilename, strerror (errno));
    }

  while ((pN = *ppList++))
    {
      pF = fopen (pN, "r");
      if (pF == NULL)
	Fatal ("%s: %s", pN, strerror (errno));

      R = stat (pN, &St);
      if (R < 0)
	Fatal ("%s: stat failed: %s", pN, strerror (errno));

      Size = sizeof (PAYLOAD_HDR) + St.st_size;

      pBuf = calloc (1, Size);
      if (pBuf == NULL)
	Fatal ("calloc failed");

      pHdr = (PAYLOAD_HDR *) pBuf;
      pHdr->magic = gMagic;
      pHdr->filename = Zoo64Encode (pN);
      pHdr->size = St.st_size;

      if (fread ((void *) (pHdr + 1), 1, St.st_size, pF) == 0 || ferror (pF))
	Fatal ("%s: fread failed", pN);
      fclose (pF);

      if ((fwrite (pBuf, 1, Size, pOut) == 0) || ferror (pOut))
	Fatal ("Can't write to output file: %s", strerror (errno));

      free (pBuf);
    }
  fclose (pOut);
  exit (0);
}

/**
  Extract archive contents.

  Extracts all files from archive to current directory.

  @param[in] pFilename  Archive filename to extract.
**/
VOID
DoExtract (
  IN CHAR8  *pFilename
  )
{
  FILE *pF;
  PAYLOAD_HDR Hdr;

  pF = fopen (pFilename, "r");
  if (pF == NULL)
    {
      Fatal ("%s:%s", pFilename, strerror (errno));
    }

  while (!(fread ((void *) &Hdr, 1, sizeof (Hdr), pF) == 0 || ferror (pF)))
    {
      FILE *pOut;
      VOID *pBuf;

      if (Hdr.magic != gMagic)
	Fatal ("Corrupted entry (Bad Magic)");
      char *pName = Zoo64Decode (Hdr.filename);
      if (pName == NULL)
	Fatal ("Failed to decode filename");

      pBuf = calloc (1, Hdr.size);
      if (pBuf == NULL)
	{
	  free (pName);
	  Fatal ("calloc failed");
	}

      pOut = fopen (pName, "w");
      if (pOut == NULL)
	{
	  char ErrMsg[256];
	  snprintf (ErrMsg, sizeof(ErrMsg), "%s:%s", pName, strerror (errno));
	  free (pName);
	  free (pBuf);
	  Fatal ("%s", ErrMsg);
	}

      if (fread (pBuf, 1, Hdr.size, pF) == 0 || ferror (pF))
	{
	  char ErrMsg[256];
	  snprintf (ErrMsg, sizeof(ErrMsg), "%s: fread failed", pName);
	  fclose (pOut);
	  free (pName);
	  free (pBuf);
	  Fatal ("%s", ErrMsg);
	}

      if ((fwrite (pBuf, 1, Hdr.size, pOut) == 0) || ferror (pOut))
	{
	  char ErrMsg[512];
	  snprintf (ErrMsg, sizeof(ErrMsg), "Can't write to output file %s: %s", pName, strerror (errno));
	  fclose (pOut);
	  free (pName);
	  free (pBuf);
	  Fatal ("%s", ErrMsg);
	}

      fclose (pOut);
      free (pBuf);
      free (pName);
    }

  if (!feof (pF))
    Fatal ("Cannot read archive: %s", strerror (errno));
}


CONST struct option gLongOptions[] = {
  {"list", no_argument, NULL, 'l'},
  {"create", no_argument, NULL, 'c'},
  {"extract", no_argument, NULL, 'x'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  {0, no_argument, 0, 0}
};

/**
  Main entry point.

  Parses command-line arguments and dispatches to appropriate operation
  (list, create, or extract).

  @param[in] Argc  Argument count.
  @param[in] Argv  Argument vector.

  @return Exit status (0 on success, 1 on error).
**/
int
main (int Argc, char *const Argv[])
{
  bool Create, Extract, List, ShowVersion;
  unsigned CmdSeen;
  char *pFilename;
  char C;

  CmdSeen = 0;
  Create = List = ShowVersion = false;

  while ((C = getopt_long (Argc, Argv, "cxlhVm:", gLongOptions, NULL)) != EOF)
    switch (C)
      {
      case 'c':
	CmdSeen++;
	Create = true;
	break;
      case 'x':
	CmdSeen++;
	Extract = true;
	break;
      case 'l':
	CmdSeen++;
	List = true;
	break;
      case 'V':
	CmdSeen++;
	ShowVersion = true;
	break;
      case 'h':
	Usage (stdout, 0);
	break;
      case 'm':
	gMagic = Zoo64Encode (optarg);
	break;
      default:
	Usage (stderr, 1);
      }

  Argc -= optind;
  Argv += optind;

  if (CmdSeen != 1)
    Usage (stderr, 1);
  if (ShowVersion)
    {
      if (Argc != 0)
	{
	  Usage (stderr, 1);
	}
      PrintVersion ();
    }
  if (Argc < 1)
    Usage (stderr, 1);
  pFilename = Argv[0];
  Argc -= 1;
  Argv += 1;

  if (Create)
    {
      if (Argc == 0)
	{
	  Usage (stderr, 1);
	}
      DoCreate (pFilename, Argv);
    }

  if (Extract)
    {
      if (Argc != 0)
	{
	  Usage (stderr, 1);
	}
      DoExtract (pFilename);
    }

  if (List)
    {
      if (Argc != 0)
	{
	  Usage (stderr, 1);
	}
      DoList (pFilename);
    }

  return 0;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use Report instead **/
void report (const char *format, va_list args) {
  Report (format, args);
}

/** @deprecated Use NonFatal instead **/
void non_fatal (const char *format, ...) {
  va_list args;
  va_start (args, format);
  NonFatal (format, args);
  va_end (args);
}

/** @deprecated Use Fatal instead **/
void fatal (const char *format, ...) {
  va_list args;
  va_start (args, format);
  Fatal (format, args);
  va_end (args);
}

/** @deprecated Use Usage instead **/
static void usage (FILE *f, int status) {
  Usage (f, status);
}

/** @deprecated Use PrintVersion instead **/
static void print_version (void) {
  PrintVersion ();
}

/** @deprecated Use DoList instead **/
void do_list (char *filename) {
  DoList (filename);
}

/** @deprecated Use DoCreate instead **/
void do_create (char *filename, char *const list[]) {
  DoCreate (filename, list);
}

/** @deprecated Use DoExtract instead **/
void do_extract (char *filename) {
  DoExtract (filename);
}
