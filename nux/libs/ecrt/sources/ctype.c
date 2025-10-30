
/* libkern.h */

int
isspace (int ch)
{
  return (ch == ' ' || (ch >= '\t' && ch <= '\r'));
}
