#!/bin/bash
#SBATCH -c 40
#SBATCH -p gpu

root_dir="$PWD"

export LD_LIBRARY_PATH="$PWD"

ulimit -S -c 0 # disable core dumps

# export GASNET_PHYSMEM_MAX=16G # hack for some reason this seems to be necessary on Piz Daint now

if [[ ! -d dcr_idx ]]; then mkdir dcr_idx; fi
pushd dcr_idx

for n in $SLURM_JOB_NUM_NODES; do
  for r in 0 1 2 3 4; do
    echo "Running $n""x1_r$r"
    mpirun -n $n -N $n -npernode 1 --bind-to none "$root_dir/circuit.idx" -npp 5000 -wpp 20000 -l 50 -p $(( $n * 40 )) -pps 10 -prune 30 -hl:sched 1024 -ll:gpu 1 -ll:util 2 -ll:bgwork 2 -ll:csize 15000 -ll:fsize 15000 -ll:zsize 2048 -ll:rsize 512 -ll:gsize 0 -level 5 -dm:replicate 1 -dm:same_address_space -dm:memoize -lg:no_fence_elision -lg:parallel_replay 2 | tee out_"$n"x1_r"$r".log
  done
done

popd

if [[ ! -d dcr_noidx ]]; then mkdir dcr_noidx; fi
pushd dcr_noidx

for n in $SLURM_JOB_NUM_NODES; do
  for r in 0 1 2 3 4; do
    echo "Running $n""x1_r$r"
    mpirun -n $n -N $n -npernode 1 --bind-to none "$root_dir/circuit.noidx" -npp 5000 -wpp 20000 -l 50 -p $(( $n * 40 )) -pps 10 -prune 30 -hl:sched 1024 -ll:gpu 4 -ll:util 2 -ll:bgwork 2 -ll:csize 15000 -ll:fsize 15000 -ll:zsize 2048 -ll:rsize 512 -ll:gsize 0 -level 5 -dm:replicate 1 -dm:same_address_space -dm:memoize -lg:no_fence_elision -lg:parallel_replay 2 | tee out_"$n"x1_r"$r".log
  done
done

popd
