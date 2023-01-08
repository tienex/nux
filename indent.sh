#!/bin/bash

set -x

for i in `find . | grep \\\.[ch]$ | grep -v contrib/`; do echo $i; gindent -gnu $i ; done 
