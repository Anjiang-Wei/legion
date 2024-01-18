#!/bin/bash
#BSUB -alloc_flags atsdisable

n=$(( ($LSB_DJOB_NUMPROC - 1) / 40))
numgpus=4

echo Num nodes: $numnodes
echo Job id: $LSB_JOBID

module load gcc/7.3.1
module load cmake/3.14.5
module load cuda/11.7.0

root_dir="$PWD"
export LD_LIBRARY_PATH="$PWD"

repeat=5
nx=NX_ARG
ny=NY_ARG

ntx=NTX_ARG
nty=NTY_ARG

tile=10000

oricmd=" -tsteps 50 -tprune 30 -hl:sched 1024 -ll:gpu ${numgpus} -ll:util 1 -ll:bgwork 2 -ll:csize 150000 -ll:fsize 15000 -ll:zsize 2048 -ll:rsize 512 -ll:gsize 0 -level 5 -dm:replicate 1 -dm:same_address_space -dm:memoize -lg:no_fence_elision -lg:parallel_replay 2 "

logori=$oricmd" -wrapper -level mapper=debug -logfile wrapper_${nx}_${ny}_${nx}_${ny}_n%.wrapper -lg:prof ${n} -lg:prof_logfile prof_${nx}_${ny}_${nx}_${ny}_n%.gz "
logcpl=$oricmd" -wrapper -level mapper=debug -logfile wrapper_${nx}_${ny}_${ntx}_${nty}_n%.wrapper -lg:prof ${n} -lg:prof_logfile prof_${nx}_${ny}_${ntx}_${nty}_n%.gz "


if [[ ! -d part_${nx}_${ny} ]]; then mkdir part_${nx}_${ny}; fi
pushd part_${nx}_${ny}

jsrun -b none -c ALL_CPUS -g ALL_GPUS -r 1 -n $n "$root_dir/stencil"  -nx $(( nx * ${tile} )) -ny $(( ny * ${tile} )) -ntx ${nx} -nty ${ny} $logori
jsrun -b none -c ALL_CPUS -g ALL_GPUS -r 1 -n $n "$root_dir/stencil"  -nx $(( nx * ${tile} )) -ny $(( ny * ${tile} )) -ntx ${ntx} -nty ${nty} $logcpl
for (( r=1; r <= ${repeat}; r++)); do
  echo "Running ${nx}_${ny}_r${r}"
  jsrun -b none -c ALL_CPUS -g ALL_GPUS -r 1 -n $n "$root_dir/stencil"  -nx $(( nx * ${tile} )) -ny $(( ny * ${tile} )) -ntx ${nx} -nty ${ny} $oricmd | tee out_${nx}_${ny}_${nx}_${ny}_r${r}.log
  jsrun -b none -c ALL_CPUS -g ALL_GPUS -r 1 -n $n "$root_dir/stencil"  -nx $(( nx * ${tile} )) -ny $(( ny * ${tile} )) -ntx ${ntx} -nty ${nty} $oricmd | tee out_${nx}_${ny}_${ntx}_${nty}_r${r}.log
done

popd

echo "bsub_stencil_part.lsf finishes!"
