#!/bin/sh
#SBATCH --nodes=8
#SBATCH --constraint=gpu
#SBATCH --time=01:00:00
#SBATCH --mail-type=ALL

root_dir="$PWD"

export LD_LIBRARY_PATH="$PWD"

if [[ ! -d tracing ]]; then mkdir tracing; fi
pushd tracing

for n in 8 4 2 1; do
    for r in 0 1 2 3 4; do
        if [[ ! -f out_"$n"x9_r"$r".log ]]; then
            echo "Running $n""x9_r$r""..."
            srun -n $n -N $n --ntasks-per-node 1 --cpu_bind none /lib64/ld-linux-x86-64.so.2 "$root_dir/circuit.spmd9" -npp 50 -wpp 200 -l 100 -p $(( 1024 * 9 )) -pps $(( 1024 / n )) -ll:csize 30000 -ll:rsize 512 -ll:gsize 0 -ll:cpu 9 -ll:io 1 -ll:util 2 -ll:dma 2 | tee out_"$n"x9_r"$r".log
            # -lg:prof 4 -lg:prof_logfile prof_"$n"x9_r"$r"_%.gz
        fi
    done
done

popd
