#!/bin/bash
# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/build_circuit.sh

set -e

root_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

mkdir "$1"
cd "$1"

# Hack: -ffuture 0 is a workaround for blocking on a future with the trace loop
# change arch to volta on lassen
SAVEOBJ=1 STANDALONE=1 OBJNAME=./circuit.idx $root_dir/../regent.py $root_dir/../examples/circuit_sparse_dsl.rg -fflow 0 -fopenmp 0 -fcuda 1 -fcuda-offline 1 -fgpu-arch volta -findex-launch 1 -ffuture 0
SAVEOBJ=1 STANDALONE=1 OBJNAME=./circuit.noidx $root_dir/../regent.py $root_dir/../examples/circuit_sparse.rg   -fflow 0 -fopenmp 0 -fcuda 1 -fcuda-offline 1 -fgpu-arch volta -findex-launch 1 -ffuture 0

cp -v $root_dir/*_circuit*.{sh,lsf} .
cp -v $root_dir/circuit_mappings .
