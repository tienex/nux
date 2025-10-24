/** @file
  Object File Payload Appender

  Utility for appending payload files to ELF object files and executables
  using the BFD (Binary File Descriptor) library. Creates a new section
  named ".objappend" containing the payload data and updates program headers.

  Used by NUX bootloader to embed kernel/user ELF images into bootloader
  binaries.

  Copyright (C) 2015-2023 Gianluca Guida

  SPDX-License-Identifier: GPL-3.0-or-later
**/

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>

#include <bfd.h>

#define PROGNAME "objappend"
#define VERSION "0.0"
#define PAYLOAD_SECTNAME ".objappend"

static asymbol **gpIsym, **gpOsym;

/**
  Report error message.

  Internal helper for formatting error messages to stderr.

  @param[in] pFormat  Printf-style format string.
  @param[in] Args     Variable argument list.
**/
void
Report (
  const char  *pFormat,
  va_list      Args
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
void
NonFatal (
  const char  *pFormat,
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
void
Fatal (
  const char  *pFormat,
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
  Report non-fatal BFD error.

  Retrieves and displays BFD library error message.

  @param[in] pString  Context string (optional, may be NULL).
**/
void
BfdNonFatal (
  const char  *pString
  )
{
  const char *pErrMsg;

  pErrMsg = bfd_errmsg (bfd_get_error ());
  fflush (stdout);
  if (pString)
    fprintf (stderr, "%s: %s: %s\n", PROGNAME, pString, pErrMsg);
  else
    fprintf (stderr, "%s: %s\n", PROGNAME, pErrMsg);
}

/**
  Report fatal BFD error and exit.

  Retrieves BFD error, displays message, and terminates program.

  @param[in] pString  Context string (optional, may be NULL).
**/
void
BfdFatal (
  const char  *pString
  )
{
  BfdNonFatal (pString);
  exit (1);
}

/**
  Copy section header.

  BFD section iterator callback that creates output section with same
  properties as input section (flags, size, VMA, LMA, alignment).

  @param[in] pIBfd    Input BFD.
  @param[in] pISection  Input section.
  @param[in] pOBfdArg   Output BFD (passed as void*).
**/
static void
CopyHdrSection (
  bfd       *pIBfd,
  asection  *pISection,
  void      *pOBfdArg
  )
{
  bfd *pOBfd = (bfd *) pOBfdArg;
  bfd_size_type Size;
  sec_ptr pOSection;

  pOSection = bfd_make_section_anyway_with_flags (pOBfd,
						 bfd_section_name (pIBfd,
								   pISection),
						 bfd_get_section_flags (pIBfd,
									pISection));
  if (pOSection == NULL)
    BfdFatal ("failed to create section");

  Size = bfd_section_size (pIBfd, pISection);
  Size = bfd_convert_section_size (pIBfd, pISection, pOBfd, Size);
  if (!bfd_set_section_size (pOBfd, pOSection, Size))
    BfdFatal ("failed to set section size");

  if (!bfd_set_section_vma (pOBfd, pOSection, bfd_section_vma (pIBfd, pISection)))
    BfdFatal ("failed to set section vma");

  pOSection->lma = pISection->lma;

  if (!bfd_set_section_alignment (pOBfd,
				  pOSection,
				  bfd_section_alignment (pIBfd, pISection)))
    BfdFatal ("failed to set section alignment");

  pOSection->entsize = pISection->entsize;

  pOSection->compress_status = pISection->compress_status;

  /* Set a link between input and output section for successive deeper
     copies. */
  pISection->output_section = pOSection;
  pISection->output_offset = 0;

  if ((pISection->flags & SEC_GROUP) != 0)
    Fatal ("SEC_GROUP not supported");

  if (!bfd_copy_private_section_data (pIBfd, pISection, pOBfd, pOSection))
    BfdFatal ("failed to copy private data");

}

/**
  Set section relocations.

  BFD section iterator callback that copies relocations from input to
  output section. Canonicalizes relocations and sets them in output.

  @param[in] pIBfd      Input BFD.
  @param[in] pISection  Input section.
  @param[in] pOBfdArg   Output BFD (passed as void*).
**/
static void
SetRelocSection (
  bfd       *pIBfd,
  asection  *pISection,
  void      *pOBfdArg
  )
{
  bfd *pOBfd = (bfd *) pOBfdArg;
  asection *pOSection = pISection->output_section;
  long RelSize;
  arelent **ppRel;
  long RelCount;

  if (pOSection == NULL)
    return;

  if ((pIBfd->flags & SEC_GROUP) != 0)
    Fatal ("WTF");

  if (RelSize == 0)
    bfd_set_reloc (pOBfd, pOSection, NULL, 0);

  RelSize = bfd_get_reloc_upper_bound (pIBfd, pISection);
  if (RelSize < 0)
    {
      /* Do not complain if the target does not support relocations.  */
      if (RelSize == -1 && bfd_get_error () == bfd_error_invalid_operation)
	RelSize = 0;
      else
	BfdFatal (NULL);
    }
  if (RelSize == 0)
    {
      bfd_set_reloc (pOBfd, pOSection, NULL, 0);
      pOSection->flags &= ~SEC_RELOC;
    }
  else
    {
      ppRel = (arelent **) malloc (RelSize);
      if (ppRel == NULL)
	Fatal ("out of memory");

      RelCount = bfd_canonicalize_reloc (pIBfd, pISection, ppRel, gpIsym);
      if (RelCount < 0)
	BfdFatal ("relocation count is negative");

      bfd_set_reloc (pOBfd, pOSection, RelCount == 0 ? NULL : ppRel, RelCount);
      if (RelCount == 0)
	{
	  pOSection->flags &= ~SEC_RELOC;
	  free (ppRel);
	}
    }
}

/**
  Set section contents.

  BFD section iterator callback that copies section data from input to
  output. Handles sections with contents (code/data sections).

  @param[in] pIBfd      Input BFD.
  @param[in] pISection  Input section.
  @param[in] pOBfdArg   Output BFD (passed as void*).
**/
static void
SetContentSection (
  bfd       *pIBfd,
  asection  *pISection,
  void      *pOBfdArg
  )
{
  bfd *pOBfd = (bfd *) pOBfdArg;
  asection *pOSection = pISection->output_section;
  struct section_list *pP;
  bfd_size_type Size = bfd_get_section_size (pISection);

  if (pOSection == NULL)
    return;

  if (bfd_get_section_flags (pIBfd, pISection) & SEC_HAS_CONTENTS
      && bfd_get_section_flags (pOBfd, pOSection) & SEC_HAS_CONTENTS)
    {
      bfd_byte *pMemHunk = NULL;

      if (!bfd_get_full_section_contents (pIBfd, pISection, &pMemHunk)
	  || !bfd_convert_section_contents (pIBfd, pISection, pOBfd,
					    &pMemHunk, &Size))
	BfdFatal (NULL);

      if (!bfd_set_section_contents (pOBfd, pOSection, pMemHunk, 0, Size))
	BfdFatal (NULL);

      free (pMemHunk);
    }
}

/**
  Get maximum VMA.

  BFD section iterator callback that tracks highest virtual address + size
  across all allocated sections.

  @param[in] pIBfd      Input BFD.
  @param[in] pISection  Input section.
  @param[in] pPtr       Pointer to unsigned long for max VMA result.
**/
static void
GetMaxVma (
  bfd       *pIBfd,
  asection  *pISection,
  void      *pPtr
  )
{
  unsigned long Vma, *pMaxVma = (unsigned long *) pPtr;

  if (!(bfd_get_section_flags (pIBfd, pISection) & SEC_ALLOC))
    return;

  Vma =
    bfd_get_section_vma (pIBfd, pISection) + bfd_section_size (pIBfd, pISection);
  if (Vma > *pMaxVma)
    *pMaxVma = Vma;
}

/**
  Get maximum LMA.

  BFD section iterator callback that tracks highest load address + size
  across all allocated sections.

  @param[in] pIBfd      Input BFD.
  @param[in] pISection  Input section.
  @param[in] pPtr       Pointer to unsigned long for max LMA result.
**/
static void
GetMaxLma (
  bfd       *pIBfd,
  asection  *pISection,
  void      *pPtr
  )
{
  unsigned long Lma, *pMaxLma = (unsigned long *) pPtr;

  if (!(bfd_get_section_flags (pIBfd, pISection) & SEC_ALLOC))
    return;

  Lma = pISection->lma + bfd_section_size (pIBfd, pISection);
  if (Lma > *pMaxLma)
    *pMaxLma = Lma;
}

/**
  Get payload addresses.

  Determines where to place payload by finding maximum VMA and LMA across
  all allocated sections in input BFD.

  @param[in]  pIBfd  Input BFD.
  @param[out] pLma   Pointer to receive maximum LMA.
  @param[out] pVma   Pointer to receive maximum VMA.
**/
void
GetPayloadAddresses (
  bfd            *pIBfd,
  unsigned long  *pLma,
  unsigned long  *pVma
  )
{
  *pLma = 0;
  *pVma = 0;
  bfd_map_over_sections (pIBfd, GetMaxLma, (void *) pLma);
  bfd_map_over_sections (pIBfd, GetMaxVma, (void *) pVma);
}

/**
  Create payload section.

  Creates a new section in output BFD to hold payload file. Section is
  allocated, loadable, read-only data positioned after all existing sections.

  @param[in] pOBfd      Output BFD.
  @param[in] pFilename  Payload filename (for size determination).
  @param[in] Lma        Load memory address.
  @param[in] Vma        Virtual memory address.

  @return Pointer to created section.
**/
asection *
CreatePayloadSection (
  bfd            *pOBfd,
  char          *pFilename,
  unsigned long  Lma,
  unsigned long  Vma
  )
{
  asection *pS;
  flagword Flags;
  struct stat St;

  Flags = SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS
    | SEC_READONLY | SEC_DATA | SEC_LINKER_CREATED;
  pS = bfd_make_section_anyway_with_flags (pOBfd, PAYLOAD_SECTNAME, Flags);
  if (pS == NULL)
    Fatal ("can't add payload section");

  if (lstat (pFilename, &St) < 0)
    Fatal ("can't stat %s", pFilename);

  if (!bfd_set_section_size (pOBfd, pS, (size_t) St.st_size))
    Fatal ("can't set payload section initial size");

  if (!bfd_set_section_alignment (pOBfd, pS, 0))
    Fatal ("can't set payload section alignment");

  if (!bfd_set_section_vma (pOBfd, pS, Vma))
    BfdFatal ("failed to set section vma");

  pS->lma = Lma;

  /*
     Set output_section to NULL, to differentiate this new payload in
     next section walks. We have in fact no input equivalent.
   */
  pS->output_section = NULL;

  return pS;
}

/**
  Fill payload section.

  Reads payload file and writes contents to payload section.

  @param[in] pOBfd      Output BFD.
  @param[in] pS         Payload section.
  @param[in] pFilename  Payload filename.
**/
void
FillPayloadSection (
  bfd       *pOBfd,
  asection  *pS,
  char     *pFilename
  )
{
  int R;
  bfd_byte *pBuf;
  FILE *pF;
  struct stat St;
  size_t Size;

  pF = fopen (pFilename, "r");
  if (pF == NULL)
    Fatal ("%s: %s", pFilename, strerror (errno));

  R = lstat (pFilename, &St);
  if (R < 0)
    Fatal ("%s: stat failed: %s", pFilename, strerror (errno));

  Size = St.st_size;
  pBuf = calloc (1, Size);
  if (pBuf == NULL)
    Fatal ("calloc failed");

  if (fread (pBuf, 1, Size, pF) == 0 || ferror (pF))
    Fatal ("%s: fread failed", pFilename);
  fclose (pF);

  if (!bfd_set_section_contents (pOBfd, pS, pBuf, 0, Size))
    BfdFatal ("setting payload contents");
  free (pBuf);
}

/**
  Copy BFD with payload.

  Poor man's objcopy implementation. Copies input BFD to output BFD,
  optionally adding a payload section. BFD library does not support
  read-modify-write, so copying is necessary to modify binaries.

  @param[in] pIBfd      Input BFD.
  @param[in] pOBfd      Output BFD.
  @param[in] pAddFile   Payload filename to add (NULL if none).
  @param[in] AddLma     Payload load address.
  @param[in] AddVma     Payload virtual address.
**/
void
CopyBfd (
  bfd            *pIBfd,
  bfd            *pOBfd,
  char          *pAddFile,
  unsigned long  AddLma,
  unsigned long  AddVma
  )
{
  long SymCount;
  long SymSize;
  flagword Flags;
  asection *pPSec;

  /*
     Set format.
   */
  if (!bfd_set_format (pOBfd, bfd_get_format (pIBfd)))
    BfdFatal (NULL);

  /*
     Check sections.
   */
  if (pIBfd->sections == NULL)
    Fatal ("file has no sections");


  /*
     Set start and flags.
   */
  Flags = bfd_get_file_flags (pIBfd);
  Flags &= bfd_applicable_file_flags (pOBfd);
  if (!bfd_set_start_address (pOBfd, bfd_get_start_address (pIBfd))
      || !bfd_set_file_flags (pOBfd, Flags))
    BfdFatal (NULL);


  /*
     Set arch and mach.
   */
  if (bfd_get_arch (pIBfd) == bfd_arch_unknown)
    Fatal ("unable to recognise the format of the input file");
  if (!bfd_set_arch_mach (pOBfd, bfd_get_arch (pIBfd), bfd_get_mach (pIBfd)))
    BfdFatal (NULL);

  if (!bfd_set_format (pOBfd, bfd_get_format (pIBfd)))
    BfdFatal (NULL);


  /*
     Copy symbols.
   */
  gpIsym = NULL;
  gpOsym = NULL;

  SymSize = bfd_get_symtab_upper_bound (pIBfd);
  if (SymSize < 0)
    BfdFatal (NULL);

  gpOsym = gpIsym = (asymbol **) malloc (SymSize);
  if (gpIsym == NULL)
    Fatal ("out of memory");

  SymCount = bfd_canonicalize_symtab (pIBfd, gpIsym);
  if (SymCount < 0)
    BfdFatal (NULL);

  if (SymCount == 0)
    {
      free (gpIsym);
      gpOsym = gpIsym = NULL;
    }


  /*
     Update sections.
   */

  /* Step 1: add sections. */
  bfd_map_over_sections (pIBfd, CopyHdrSection, pOBfd);

  /* Step 3: add new payload section */
  if (pAddFile != NULL)
    pPSec = CreatePayloadSection (pOBfd, pAddFile, AddLma, AddVma);

  /* Step 2: copy header data. */
  if (!bfd_copy_private_header_data (pIBfd, pOBfd))
    BfdFatal ("error in private header data");

  /* Step 4: set symbols. */
  bfd_set_symtab (pOBfd, gpOsym, SymCount);

  bfd_record_phdr (pOBfd, 1, false, 0, false, 0, false, false, 1, &pPSec);

  /* Step 5: copy relocations. */
  bfd_map_over_sections (pIBfd, SetRelocSection, pOBfd);

  /* Step 6: copy contents. */
  bfd_map_over_sections (pIBfd, SetContentSection, pOBfd);

  /* Step 7: set payload contents */
  if (pAddFile != NULL)
    FillPayloadSection (pOBfd, pPSec, pAddFile);

  if (!bfd_copy_private_bfd_data (pIBfd, pOBfd))
    BfdFatal ("error copying private data");
}

/**
  Add payloads to executable.

  Main operation: iterates through payload file list, opening executable,
  adding each payload, and writing back to same file.

  @param[in] pFilename  Executable filename.
  @param[in] ppList     NULL-terminated array of payload filenames.
**/
static void
DoAdd (
  char         *pFilename,
  char *const  ppList[]
  )
{
  char *pN;

  bfd_init ();

  while ((pN = *(ppList++)))
    {
      bfd *pIBfd, *pOBfd;
      unsigned long Lma, Vma;
      char **ppObjMatching;

      pIBfd = bfd_openr (pFilename, NULL);
      if (pIBfd == NULL)
	Fatal ("%s: %s", pFilename, bfd_errmsg (bfd_get_error ()));

      if (!bfd_check_format_matches (pIBfd, bfd_object, &ppObjMatching))
	Fatal ("%s: Not an object or executable", pFilename);

      pOBfd = bfd_openw (pFilename, bfd_get_target (pIBfd));
      if (pOBfd == NULL)
	Fatal ("%s: %s", pN, bfd_errmsg (bfd_get_error ()));

      GetPayloadAddresses (pIBfd, &Lma, &Vma);

      CopyBfd (pIBfd, pOBfd, pN, Lma, Vma);

      if (!bfd_close (pOBfd))
	BfdFatal ("bfd_close");
    }
}

/**
  Print usage information.

  Displays command-line usage and available options.

  @param[in] pF      Output file stream (stdout or stderr).
  @param[in] Status  Exit status code.
**/
static void
Usage (
  FILE  *pF,
  int   Status
  )
{
  fprintf (pF, "Usage: %s [command]\n", PROGNAME);
  fprintf (pF, " Command is one of the following:\n\
  {-a|--add} exec [<file>...]  Append files to EXEC\n\
  {-h|--help}                  Display this information\n\
  {-V|--version}               Display this program's version number\n \
\n");
  exit (Status);
}

/**
  Print version information.

  Displays program name, version, copyright, and license.
**/
static void
PrintVersion (
  void
  )
{
  printf ("%s %s\n", PROGNAME, VERSION);
  printf ("Copyright (C) 2015-2023 Gianluca Guida.\n");
  printf ("\
This program is free software; you may redistribute it under the terms of\n\
the GNU General Public License version 3 or (at your option) any later version.\n\
This program has absolutely no warranty.\n");
  exit (0);
}

const struct option gLongOptions[] = {
  {"add", 1, NULL, 'a'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  {0, no_argument, 0, 0}
};


/**
  Main entry point.

  Parses command-line arguments and dispatches to add operation.

  @param[in] Argc  Argument count.
  @param[in] Argv  Argument vector.

  @return Exit status (0 on success, 1 on error).
**/
int
main (int Argc, char *const Argv[])
{
  int C;
  unsigned CmdSeen;
  char *pFilename;
  bool RdOnly;
  bool Add, ShowVersion;

  CmdSeen = 0;
  ShowVersion = false;
  while ((C = getopt_long (Argc, Argv, "ahV", gLongOptions, NULL)) != EOF)
    {
      switch (C)
	{
	case 'a':
	  Add = true;
	  CmdSeen++;
	  break;
	case 'V':
	  CmdSeen++;
	  ShowVersion = true;
	  break;
	case 'h':
	  Usage (stdout, 0);
	  break;
	default:
	  Usage (stderr, 1);
	}
    }

  if (CmdSeen != 1)
    Usage (stderr, 1);

  Argc -= optind;
  Argv += optind;

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


  if (Add)
    {
      if (Argc == 0)
	{
	  Usage (stderr, 1);
	}
      DoAdd (pFilename, Argv);
    }
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

/** @deprecated Use BfdNonFatal instead **/
void bfd_nonfatal (const char *string) {
  BfdNonFatal (string);
}

/** @deprecated Use BfdFatal instead **/
void bfd_fatal (const char *string) {
  BfdFatal (string);
}

/** @deprecated Use CopyHdrSection instead **/
static void _copyhdr_section (bfd *ibfd, asection *isection, void *obfdarg) {
  CopyHdrSection (ibfd, isection, obfdarg);
}

/** @deprecated Use SetRelocSection instead **/
static void _setreloc_section (bfd *ibfd, asection *isection, void *obfdarg) {
  SetRelocSection (ibfd, isection, obfdarg);
}

/** @deprecated Use SetContentSection instead **/
static void _setcontent_section (bfd *ibfd, asection *isection, void *obfdarg) {
  SetContentSection (ibfd, isection, obfdarg);
}

/** @deprecated Use GetMaxVma instead **/
static void _get_max_vma (bfd *ibfd, asection *isection, void *ptr) {
  GetMaxVma (ibfd, isection, ptr);
}

/** @deprecated Use GetMaxLma instead **/
static void _get_max_lma (bfd *ibfd, asection *isection, void *ptr) {
  GetMaxLma (ibfd, isection, ptr);
}

/** @deprecated Use GetPayloadAddresses instead **/
void get_payload_addresses (bfd *ibfd, unsigned long *lma, unsigned long *vma) {
  GetPayloadAddresses (ibfd, lma, vma);
}

/** @deprecated Use CreatePayloadSection instead **/
asection *create_payload_section (bfd *obfd, char *filename,
				  unsigned long lma, unsigned long vma) {
  return CreatePayloadSection (obfd, filename, lma, vma);
}

/** @deprecated Use FillPayloadSection instead **/
void fill_payload_section (bfd *obfd, asection *s, char *filename) {
  FillPayloadSection (obfd, s, filename);
}

/** @deprecated Use CopyBfd instead **/
void copy_bfd (bfd *ibfd, bfd *obfd,
	      char *add_file, unsigned long add_lma, unsigned long add_vma) {
  CopyBfd (ibfd, obfd, add_file, add_lma, add_vma);
}

/** @deprecated Use DoAdd instead **/
static void do_add (char *filename, char *const list[]) {
  DoAdd (filename, list);
}

/** @deprecated Use Usage instead **/
static void usage (FILE *f, int status) {
  Usage (f, status);
}

/** @deprecated Use PrintVersion instead **/
static void print_version (void) {
  PrintVersion ();
}
