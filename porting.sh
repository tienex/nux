#!/bin/bash

rm example.ar50
cp apxh/sbi/apxh kernel
tools/ar50/ar50 -m nux-payload -c example.ar50 porting/0_empty/empty
tools/objappend/objappend -a kernel example.ar50
