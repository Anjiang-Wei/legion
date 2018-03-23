#!/bin/bash
#MSUB -l nodes=64
#MSUB -l walltime=0:30:00
#MSUB -q pbatch
#MSUB -m abe

root_dir="$PWD"

export LD_LIBRARY_PATH="$PWD"

export GASNET_SPAWNER=pmi

export REALM_BACKTRACE=1

if [[ ! -d tracing ]]; then mkdir tracing; fi
pushd tracing

for n in 64 32 16; do
    for r in 0 1 2 3 4; do
        if [[ ! -f out_"$n"x14_r"$r".log ]]; then
            echo "Running $n""x14_r$r""..."

            OMP_NUM_THREADS=36 srun -n $(( n * 2 )) -N $n "$root_dir"/taylor-256x128x128-dop"$n" -ll:csize 30000 -ll:rsize 1024 -ll:gsize 0 -ll:cpu 0 -ll:ocpu 1 -ll:othr 14 -ll:okindhack -ll:io 1 -ll:util 1 -ll:dma 2 -dm:memoize | tee out_"$n"x14_r"$r".log
            # -lg:prof 4 -lg:prof_logfile prof_"$n"x14_r"$r"_%.gz
        fi
    done
done

popd
