#!/bin/sh
aclocal && autoconf
(cd apxh; aclocal && autoconf)
(cd nux/example; aclocal && autoconf)
