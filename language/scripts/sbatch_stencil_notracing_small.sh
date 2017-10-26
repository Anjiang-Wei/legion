#!/bin/sh
#SBATCH --nodes=8
#SBATCH --constraint=gpu
#SBATCH --time=00:30:00
#SBATCH --mail-type=ALL

root_dir="$PWD"

export LD_LIBRARY_PATH="$PWD"

if [[ ! -d notracing ]]; then mkdir notracing; fi
pushd notracing

for i in 3 2 1 0; do
    n=$(( 2 ** i))
    nx=$(( 2 ** ((i+1)/2) ))
    ny=$(( 2 ** (i/2) ))
    for r in 0 1 2 3 4; do
        if [[ ! -f out_"$n"x10_r"$r".log ]]; then
            echo "Running $n""x10_r$r"" ($n = $nx * $ny)..."
	    srun -n $n -N $n --ntasks-per-node 1 --cpu_bind none /lib64/ld-linux-x86-64.so.2 "$root_dir/stencil.spmd10" -nx 40000 -ny 40000 -ntx $(( nx * 2 )) -nty $(( ny * 5 )) -tsteps 100 -tprune 5 -ll:cpu 10 -ll:io 1 -ll:util 1 -ll:dma 2 -ll:csize 30000 -ll:rsize 512 -ll:gsize 0 -lg:no_physical_tracing -hl:prof 4 -hl:prof_logfile prof_"$n"x10_r"$r"_%.gz | tee out_"$n"x10_r"$r".log
        fi
    done
done

popd
