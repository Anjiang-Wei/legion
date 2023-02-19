#!/bin/bash

root_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

mkdir "$1"
cd "$1"

# Hack: -ffuture 0 is a workaround for blocking on a future with the trace loop
build_option="-fflow 0 -fopenmp 0 -fcuda 0 -fgpu-arch volta -findex-launch 1"
SAVEOBJ=1 STANDALONE=1 OBJNAME=./cholesky $root_dir/../regent.py $root_dir/../examples/cholesky.rg $build_option

cp -v $root_dir/*cholesky* .
echo "build_cholesky.sh done"