#!/bin/bash
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -c 40
#SBATCH -p gpu

srun ./circuit -mapping mappings -level nsmapper=debug -ll:gpu 1
