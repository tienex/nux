#!/bin/sh
aclocal && autoconf
(cd apxh; aclocal && autoconf)
(cd example; aclocal && autoconf)
