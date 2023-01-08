#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static int
chenc (char c)
{
  int r;

  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 1;
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 1;
  if (c >= '0' && c <= '9')
    return c - '0' + 036;

  return c == ' ' ? 0 : c == '$' ? 033 : c == '.' ? 034 : 035;
}

static char
chdec (int s40)
{
  if (!s40)
    return ' ';
  if (s40 <= 032)
    return 'A' + s40 - 1;
  if (s40 >= 036)
    return '0' + s40 - 036;

  return s40 == 033 ? '$' : s40 == 034 ? '.' : '_';
}

uint64_t
squoze (char *string)
{
  char *ptr = string, *max = ptr + 12;
  uint64_t sqz = 0;

  while ((*ptr != '\0') && (ptr < max))
    {
      sqz *= 40;
      sqz += chenc (*ptr++);
    }

  /* Pad, to get first character at top bits */
  while (ptr++ < max)
    sqz *= 40;

  return sqz;
}

size_t
unsquozelen (uint64_t enc, size_t len, char *string)
{
  uint64_t cut = 1LL * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40 * 40;
  char *ptr = string;

  while (enc && len-- > 0)
    {
      *ptr++ = chdec ((enc / cut) % 40);
      enc = (enc % cut) * 40;
    }
  if (len)
    memset (ptr, 0, len);
  return ptr - string;
}

char *
unsquoze (uint64_t enc)
{
  char *str = malloc (13);
  if (str == NULL)
    return NULL;

  unsquozelen (enc, 13, str);
  str[12] = '\0';
  return str;
}
