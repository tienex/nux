/** @file
  RAD-50 Decoding Utility

  Command-line utility that reads RAD-50 encoded hexadecimal values from
  stdin and outputs decoded ASCII strings. Reverse operation of sq utility.

  Copyright (C) 2015-2023 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "squoze.h"

/**
  Main entry point.

  Reads hexadecimal lines from stdin, decodes each RAD-50 value, and
  outputs the decoded ASCII string.

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
  long long Sq;

  (void) Argc;
  (void) Argv;

  while ((LineLen = getline (&pLine, &LineCap, stdin)) > 0)
    {
      if (pLine[LineLen - 1] == '\n')
	pLine[LineLen - 1] = '\0';

      Sq = strtoull (pLine, NULL, 16);
      char *pStr = unsquoze (Sq);
      printf ("%s\n", pStr);
      free (pStr);
    }

  free (pLine);
}
