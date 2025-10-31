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
#include "solid.h"

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
BOOLEAN gSolidMode = FALSE;
UINT32 gWindowSize = DEFAULT_WINDOW_SIZE;

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
  -s, --solid		Use solid compression (compress all files together).\n\
  -w <size>		Window size for solid compression (4K,8K,16K,32K,64K,128K,256K,512K,1M).\n\
			Default: 64K\n\
\n\
 Features:\n\
  - Zoo64 format: Fixed-point range arithmetic adaptive encoding\n\
  - Supports up to 16 character filenames (vs 12 in AR50)\n\
  - Full printable ASCII support (space through tilde)\n\
  - Better compression than RAD-50 encoding\n\
  - Solid mode: compress multiple files together for better compression\n\
  - Windowed compression: configurable window sizes (4K-1M) with second window\n\
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

      BOOLEAN IsCompressed = (Hdr.flags & FLAG_COMPRESSED) != 0;
      if (IsCompressed)
	fprintf (stdout, "%16s: %-10u (comp) %-10u (orig) %08lx\n",
		 pName, Hdr.size, Hdr.orig_size, ftell (pF));
      else
	fprintf (stdout, "%16s: %-10u %08lx\n", pName, Hdr.size, ftell (pF));

      free (pName);
      fseek (pF, Hdr.size, SEEK_CUR);
    }

  if (!feof (pF))
    Fatal ("Cannot read archive: %s", strerror (errno));
}

/**
  Create new solid archive.

  Creates solid archive where all files are compressed together.

  @param[in] pFilename  Archive filename to create.
  @param[in] ppList     NULL-terminated array of filenames to archive.
**/
VOID
DoCreateSolid (
  IN CHAR8         *pFilename,
  IN CHAR8 *CONST  ppList[]
  )
{
  FILE *pF, *pOut;
  CHAR8 *pN;
  UINT8 **ppFileData;
  UINT32 *pFileSizes;
  UINT64 *pFilenames;
  UINT32 FileCount;
  struct stat St;
  UINT8 *pSolidData;
  size_t SolidSize;
  UINT32 TotalOrigSize = 0;

  // Count files
  FileCount = 0;
  CHAR8 *CONST *pTemp = ppList;
  while (*pTemp++)
    FileCount++;

  if (FileCount == 0)
    Fatal ("No files to archive");

  // Allocate arrays
  ppFileData = (UINT8 **) calloc (FileCount, sizeof (UINT8 *));
  pFileSizes = (UINT32 *) calloc (FileCount, sizeof (UINT32));
  pFilenames = (UINT64 *) calloc (FileCount, sizeof (UINT64));

  if (ppFileData == NULL || pFileSizes == NULL || pFilenames == NULL)
    Fatal ("calloc failed");

  // Load all files
  for (UINT32 I = 0; I < FileCount; I++)
    {
      pN = ppList[I];

      if (stat (pN, &St) < 0)
        Fatal ("%s: stat failed: %s", pN, strerror (errno));

      pFileSizes[I] = St.st_size;
      pFilenames[I] = Zoo64Encode (pN);
      TotalOrigSize += pFileSizes[I];

      if (pFileSizes[I] > 0)
        {
          ppFileData[I] = (UINT8 *) calloc (1, pFileSizes[I]);
          if (ppFileData[I] == NULL)
            Fatal ("calloc failed");

          pF = fopen (pN, "r");
          if (pF == NULL)
            Fatal ("%s: %s", pN, strerror (errno));

          if (fread (ppFileData[I], 1, pFileSizes[I], pF) == 0 || ferror (pF))
            Fatal ("%s: fread failed", pN);

          fclose (pF);
        }
      else
        {
          ppFileData[I] = NULL;
        }
    }

  // Allocate solid compression buffer
  pSolidData = (UINT8 *) calloc (1, TotalOrigSize * 3 + 8192);
  if (pSolidData == NULL)
    Fatal ("calloc failed for solid compression buffer");

  // Compress all files together
  fprintf (stdout, "Solid compressing %u files (total: %u bytes) with %uK window...\n",
           FileCount, TotalOrigSize, gWindowSize / 1024);

  if (!SolidCompress ((const UINT8 **)ppFileData, pFileSizes, pFilenames,
                      FileCount, gWindowSize, pSolidData,
                      TotalOrigSize * 3 + 8192, &SolidSize))
    {
      Fatal ("Solid compression failed");
    }

  fprintf (stdout, "Solid: %u -> %zu bytes (%.1f%%)\n",
           TotalOrigSize, SolidSize, 100.0 * SolidSize / TotalOrigSize);

  // Write solid archive
  pOut = fopen (pFilename, "w");
  if (pOut == NULL)
    Fatal ("%s: %s", pFilename, strerror (errno));

  if (fwrite (pSolidData, 1, SolidSize, pOut) == 0 || ferror (pOut))
    Fatal ("Can't write to output file: %s", strerror (errno));

  fclose (pOut);

  // Cleanup
  for (UINT32 I = 0; I < FileCount; I++)
    {
      if (ppFileData[I])
        free (ppFileData[I]);
    }
  free (ppFileData);
  free (pFileSizes);
  free (pFilenames);
  free (pSolidData);

  exit (0);
}

/**
  Extract solid archive contents.

  Extracts all files from solid archive to current directory.

  @param[in] pFilename  Archive filename to extract.
**/
VOID
DoExtractSolid (
  IN CHAR8  *pFilename
  )
{
  FILE *pF, *pOut;
  UINT8 *pSolidData;
  size_t SolidSize;
  struct stat St;
  UINT8 **ppFileData;
  UINT32 *pFileSizes;
  UINT64 *pFilenames;
  UINT32 FileCount;

  pF = fopen (pFilename, "r");
  if (pF == NULL)
    Fatal ("%s: %s", pFilename, strerror (errno));

  // Get file size
  if (stat (pFilename, &St) < 0)
    Fatal ("%s: stat failed: %s", pFilename, strerror (errno));

  SolidSize = St.st_size;

  // Read solid archive
  pSolidData = (UINT8 *) calloc (1, SolidSize);
  if (pSolidData == NULL)
    Fatal ("calloc failed");

  if (fread (pSolidData, 1, SolidSize, pF) == 0 || ferror (pF))
    Fatal ("%s: fread failed", pFilename);

  fclose (pF);

  // Allocate arrays for extracted files (max 256 files)
  ppFileData = (UINT8 **) calloc (256, sizeof (UINT8 *));
  pFileSizes = (UINT32 *) calloc (256, sizeof (UINT32));
  pFilenames = (UINT64 *) calloc (256, sizeof (UINT64));

  if (ppFileData == NULL || pFileSizes == NULL || pFilenames == NULL)
    Fatal ("calloc failed");

  // Decompress solid archive
  if (!SolidDecompress (pSolidData, SolidSize, ppFileData, pFileSizes,
                        pFilenames, &FileCount, 256))
    {
      Fatal ("Solid decompression failed");
    }

  fprintf (stdout, "Extracted %u files from solid archive\n", FileCount);

  // Write extracted files
  for (UINT32 I = 0; I < FileCount; I++)
    {
      char *pName = Zoo64Decode (pFilenames[I]);
      if (pName == NULL)
        Fatal ("Failed to decode filename");

      fprintf (stdout, "%s: %u bytes\n", pName, pFileSizes[I]);

      pOut = fopen (pName, "w");
      if (pOut == NULL)
        Fatal ("%s: %s", pName, strerror (errno));

      if (pFileSizes[I] > 0)
        {
          if (fwrite (ppFileData[I], 1, pFileSizes[I], pOut) == 0 || ferror (pOut))
            Fatal ("%s: fwrite failed", pName);
        }

      fclose (pOut);
      free (pName);

      if (ppFileData[I])
        free (ppFileData[I]);
    }

  // Cleanup
  free (ppFileData);
  free (pFileSizes);
  free (pFilenames);
  free (pSolidData);

  exit (0);
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
  VOID *pBuf, *pCompBuf;
  size_t Size, CompSize, OrigSize;
  struct stat St;
  PAYLOAD_HDR *pHdr;
  BOOLEAN UseCompression;

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

      OrigSize = St.st_size;

      // Allocate buffer for original data
      pBuf = calloc (1, OrigSize);
      if (pBuf == NULL)
	Fatal ("calloc failed");

      if (fread (pBuf, 1, OrigSize, pF) == 0 || ferror (pF))
	{
	  free (pBuf);
	  Fatal ("%s: fread failed", pN);
	}
      fclose (pF);

      // Try compression
      UseCompression = FALSE;
      CompSize = 0;
      pCompBuf = NULL;

      if (OrigSize > 0)
	{
	  // Allocate compression buffer (extra space for range encoding frequency table + headers)
	  // Range encoding adds ~1KB overhead, plus LZ78 may expand small files
	  size_t CompBufSize = OrigSize * 3 + 3072;
	  pCompBuf = calloc (1, CompBufSize);
	  if (pCompBuf != NULL)
	    {
	      if (CompressFull ((const UINT8 *)pBuf, OrigSize, (UINT8 *)pCompBuf,
				CompBufSize, &CompSize))
		{
		  // Use compression only if it actually reduces size
		  if (CompSize < OrigSize)
		    {
		      UseCompression = TRUE;
		      fprintf (stdout, "%s: %zu -> %zu bytes (%.1f%%)\n",
			       pN, OrigSize, CompSize,
			       100.0 * CompSize / OrigSize);
		    }
		}
	    }
	}

      // Prepare header
      pHdr = (PAYLOAD_HDR *) calloc (1, sizeof (PAYLOAD_HDR));
      if (pHdr == NULL)
	{
	  free (pBuf);
	  if (pCompBuf) free (pCompBuf);
	  Fatal ("calloc failed");
	}

      pHdr->magic = gMagic;
      pHdr->filename = Zoo64Encode (pN);
      pHdr->flags = UseCompression ? FLAG_COMPRESSED : 0;
      pHdr->size = UseCompression ? CompSize : OrigSize;
      pHdr->orig_size = UseCompression ? OrigSize : 0;

      // Write header
      if ((fwrite (pHdr, 1, sizeof (PAYLOAD_HDR), pOut) == 0) || ferror (pOut))
	{
	  free (pHdr);
	  free (pBuf);
	  if (pCompBuf) free (pCompBuf);
	  Fatal ("Can't write header to output file: %s", strerror (errno));
	}
      free (pHdr);

      // Write data (compressed or original)
      if (UseCompression)
	{
	  if ((fwrite (pCompBuf, 1, CompSize, pOut) == 0) || ferror (pOut))
	    {
	      free (pBuf);
	      free (pCompBuf);
	      Fatal ("Can't write compressed data to output file: %s", strerror (errno));
	    }
	  free (pCompBuf);
	}
      else
	{
	  if ((fwrite (pBuf, 1, OrigSize, pOut) == 0) || ferror (pOut))
	    {
	      free (pBuf);
	      if (pCompBuf) free (pCompBuf);
	      Fatal ("Can't write data to output file: %s", strerror (errno));
	    }
	  if (pCompBuf) free (pCompBuf);
	}

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
      VOID *pBuf, *pDecompBuf;
      size_t WriteSize, DecompSize;
      BOOLEAN IsCompressed;

      if (Hdr.magic != gMagic)
	Fatal ("Corrupted entry (Bad Magic)");
      char *pName = Zoo64Decode (Hdr.filename);
      if (pName == NULL)
	Fatal ("Failed to decode filename");

      IsCompressed = (Hdr.flags & FLAG_COMPRESSED) != 0;

      // Allocate buffer for compressed/stored data
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

      // Read compressed/stored data from archive
      if (fread (pBuf, 1, Hdr.size, pF) == 0 || ferror (pF))
	{
	  char ErrMsg[256];
	  snprintf (ErrMsg, sizeof(ErrMsg), "%s: fread failed", pName);
	  fclose (pOut);
	  free (pName);
	  free (pBuf);
	  Fatal ("%s", ErrMsg);
	}

      // Decompress if necessary
      if (IsCompressed)
	{
	  // Allocate buffer for decompressed data
	  pDecompBuf = calloc (1, Hdr.orig_size);
	  if (pDecompBuf == NULL)
	    {
	      char ErrMsg[256];
	      snprintf (ErrMsg, sizeof(ErrMsg), "calloc failed for decompression");
	      fclose (pOut);
	      free (pName);
	      free (pBuf);
	      Fatal ("%s", ErrMsg);
	    }

	  // Decompress
	  if (!DecompressFull ((const UINT8 *)pBuf, Hdr.size, (UINT8 *)pDecompBuf,
			       Hdr.orig_size, &DecompSize))
	    {
	      char ErrMsg[256];
	      snprintf (ErrMsg, sizeof(ErrMsg), "%s: decompression failed", pName);
	      fclose (pOut);
	      free (pName);
	      free (pBuf);
	      free (pDecompBuf);
	      Fatal ("%s", ErrMsg);
	    }

	  if (DecompSize != Hdr.orig_size)
	    {
	      char ErrMsg[256];
	      snprintf (ErrMsg, sizeof(ErrMsg), "%s: decompressed size mismatch (expected %u, got %zu)",
			pName, Hdr.orig_size, DecompSize);
	      fclose (pOut);
	      free (pName);
	      free (pBuf);
	      free (pDecompBuf);
	      Fatal ("%s", ErrMsg);
	    }

	  fprintf (stdout, "%s: %u -> %zu bytes (decompressed)\n",
		   pName, Hdr.size, DecompSize);

	  // Write decompressed data
	  WriteSize = DecompSize;
	  if ((fwrite (pDecompBuf, 1, WriteSize, pOut) == 0) || ferror (pOut))
	    {
	      char ErrMsg[512];
	      snprintf (ErrMsg, sizeof(ErrMsg), "Can't write to output file %s: %s",
			pName, strerror (errno));
	      fclose (pOut);
	      free (pName);
	      free (pBuf);
	      free (pDecompBuf);
	      Fatal ("%s", ErrMsg);
	    }

	  free (pDecompBuf);
	}
      else
	{
	  // Write uncompressed data
	  WriteSize = Hdr.size;
	  if ((fwrite (pBuf, 1, WriteSize, pOut) == 0) || ferror (pOut))
	    {
	      char ErrMsg[512];
	      snprintf (ErrMsg, sizeof(ErrMsg), "Can't write to output file %s: %s",
			pName, strerror (errno));
	      fclose (pOut);
	      free (pName);
	      free (pBuf);
	      Fatal ("%s", ErrMsg);
	    }
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
  {"solid", no_argument, NULL, 's'},
  {"window", required_argument, NULL, 'w'},
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

  while ((C = getopt_long (Argc, Argv, "cxlhVm:sw:", gLongOptions, NULL)) != EOF)
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
      case 's':
	gSolidMode = TRUE;
	break;
      case 'w':
	// Parse window size
	if (strcmp (optarg, "4K") == 0 || strcmp (optarg, "4k") == 0)
	  gWindowSize = WINDOW_4K;
	else if (strcmp (optarg, "8K") == 0 || strcmp (optarg, "8k") == 0)
	  gWindowSize = WINDOW_8K;
	else if (strcmp (optarg, "16K") == 0 || strcmp (optarg, "16k") == 0)
	  gWindowSize = WINDOW_16K;
	else if (strcmp (optarg, "32K") == 0 || strcmp (optarg, "32k") == 0)
	  gWindowSize = WINDOW_32K;
	else if (strcmp (optarg, "64K") == 0 || strcmp (optarg, "64k") == 0)
	  gWindowSize = WINDOW_64K;
	else if (strcmp (optarg, "128K") == 0 || strcmp (optarg, "128k") == 0)
	  gWindowSize = WINDOW_128K;
	else if (strcmp (optarg, "256K") == 0 || strcmp (optarg, "256k") == 0)
	  gWindowSize = WINDOW_256K;
	else if (strcmp (optarg, "512K") == 0 || strcmp (optarg, "512k") == 0)
	  gWindowSize = WINDOW_512K;
	else if (strcmp (optarg, "1M") == 0 || strcmp (optarg, "1m") == 0)
	  gWindowSize = WINDOW_1M;
	else
	  Fatal ("Invalid window size: %s (valid: 4K,8K,16K,32K,64K,128K,256K,512K,1M)", optarg);
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
      if (gSolidMode)
	DoCreateSolid (pFilename, Argv);
      else
	DoCreate (pFilename, Argv);
    }

  if (Extract)
    {
      if (Argc != 0)
	{
	  Usage (stderr, 1);
	}
      if (gSolidMode)
	DoExtractSolid (pFilename);
      else
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
