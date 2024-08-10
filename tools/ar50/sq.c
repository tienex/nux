#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "squoze.h"

int
main (int argc, char *argv[])
{
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  (void) argc;
  (void) argv;

  while ((linelen = getline (&line, &linecap, stdin)) > 0)
    {
      if (line[linelen - 1] == '\n')
	line[linelen - 1] = '\0';
      uint64_t sq = squoze (line);
      printf ("%" PRIx64 "\n", sq);
    }

  free (line);
}
