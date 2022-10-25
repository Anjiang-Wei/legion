#!/bin/bash

# https://raw.githubusercontent.com/StanfordLegion/legion/papers/index-launch-sc21/language/sc21_scripts/build_pennant.sh

set -e

root_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

mkdir "$1"
cd "$1"

# change arch to volta on lassen

SAVEOBJ=1 STANDALONE=1 OBJNAME=./pennant.idx $root_dir/../regent.py $root_dir/../examples/pennant_dsl.rg -fflow 0 -fopenmp 0 -fcuda 1 -fcuda-offline 1 -fcuda-arch volta -findex-launch 1
SAVEOBJ=1 STANDALONE=1 OBJNAME=./pennant.noidx $root_dir/../regent.py $root_dir/../examples/pennant.rg   -fflow 0 -fopenmp 0 -fcuda 1 -fcuda-offline 1 -fcuda-arch volta -findex-launch 1

cp -v $root_dir/*_pennant*.{sh,lsf} .
cp -v $root_dir/pennant_mappings .
