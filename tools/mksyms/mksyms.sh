#!/bin/sh

nm -n $1 | awk '
BEGIN {
	print "#include <nux/types.h>"
	print ""
	print "const struct nux_ksym __ksyms[] = {"
}

/^[0-9a-fA-F]+ [tT]/ {
	printf("    { 0x%s, \"%s\" },\n", $1, $3)
}

END {
	printf("    { 0x0, NULL },\n", $1, $3)
	print "};"
}
' > $2
