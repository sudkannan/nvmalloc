#!/bin/sh
PINPATH=""

LD_LIBRARY_PATH=/usr/local/lib $PINPATH/pin -t  $PINPATH/source/tools/SimpleExamples/obj-intel64/pinatrace.so -- ./varname_alloc_test
