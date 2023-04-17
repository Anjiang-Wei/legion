#!/bin/bash

root_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

mkdir "$1"
cd "$1"

# Hack: -ffuture 0 is a workaround for blocking on a future with the trace loop
build_option="-fflow 0 -fopenmp 0 -foverride-demand-cuda 1 -fcuda 1 -fcuda-offline 1 -fgpu-arch volta -findex-launch 1 -ffuture 0"
SAVEOBJ=1 STANDALONE=1 OBJNAME=./stencil $root_dir/../regent.py $root_dir/../examples/stencil_fast.rg $build_option

cp -v $root_dir/*stencil* .
echo "build_stencil.sh done"