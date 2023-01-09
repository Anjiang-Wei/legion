#!/bin/sh
#SBATCH -N 2
#SBATCH -n 2
#SBATCH -c 40
#SBATCH -p gpu

export LD_LIBRARY_PATH="$PWD"

mpirun --bind-to none pennant pennant.tests/leblanc_long1x30/leblanc.pnt -npieces 4 -numpcx 1 -numpcy 4 -seq_init 0 -par_init 1 -prune 30 -hl:sched 1024 -ll:gpu 4 -ll:util 2 -ll:bgwork 2 -ll:csize 150000 -ll:fsize 15000 -ll:zsize 2048 -ll:rsize 512 -ll:gsize 0 -level mapper=debug -logfile mapper24_ori%.log -wrapper

GASNET_BACKTRACE=1 mpirun --bind-to none pennant pennant.tests/leblanc_long1x30/leblanc.pnt -npieces 4 -numpcx 1 -numpcy 4 -seq_init 0 -par_init 1 -prune 30 -hl:sched 1024 -ll:gpu 4 -ll:util 2 -ll:bgwork 2 -ll:csize 150000 -ll:fsize 15000 -ll:zsize 2048 -ll:rsize 512 -ll:gsize 0 -level mapper=debug -logfile mapper24_dsl%.log -wrapper -dslmapper -mapping pennant_mappings -tm:select_source_by_bandwidth
