/** @file
  RAD-50 Encoding Utility

  Command-line utility that reads ASCII lines from stdin and outputs
  RAD-50 encoded hexadecimal values. Used for encoding filenames and
  short strings to RAD-50 format.

  Copyright (C) 2015-2023 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "squoze.h"

/**
  Main entry point.

  Reads lines from stdin, encodes each line to RAD-50, and outputs
  the encoded value as hexadecimal.

  @param[in] Argc  Argument count (unused).
  @param[in] Argv  Argument vector (unused).

  @return Exit status (implicit 0).
**/
int
main (int Argc, char *Argv[])
{
  char *pLine = NULL;
  size_t LineCap = 0;
  ssize_t LineLen;

  (void) Argc;
  (void) Argv;

  while ((LineLen = getline (&pLine, &LineCap, stdin)) > 0)
    {
      if (pLine[LineLen - 1] == '\n')
	pLine[LineLen - 1] = '\0';
      UINT64 Sq = squoze (pLine);
      printf ("%" PRIx64 "\n", Sq);
    }

  free (pLine);
}
