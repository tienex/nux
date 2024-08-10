#include <stdint.h>

/*
  Encode a string in DEC RAD-50.
*/
uint64_t squoze (char *string);

/*
  Decode 'len' maximum characters from 'enc' into 'string'.
*/
size_t unsquozelen (uint64_t enc, size_t en, char *string);

/*
  Decode a DEC RAD-50 string into an allocated buffer.
*/
char *unsquoze (uint64_t enc);
