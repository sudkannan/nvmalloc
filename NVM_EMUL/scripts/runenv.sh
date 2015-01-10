#!/bin/bash


rootdir="/home/volos/workspace/nvmemul"
bindir=$rootdir"/build"
export LD_PRELOAD=$bindir"/src/lib/libnvmemul.so"
export NVMEMUL_INI=$rootdir"/nvmemul.ini"

echo $LD_PRELOAD
echo $NVMEMUL_INI

$@
