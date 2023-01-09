module load gcc/7.3.1
module load cmake/3.14.5
module load cuda/11.7.0
CC=/usr/tce/packages/gcc/gcc-7.3.1/bin/gcc CXX=/usr/tce/packages/gcc/gcc-7.3.1/bin/g++ USE_CUDA=1 USE_GASNET=1 CONDUIT=ibv ./scripts/setup_env.py --terra-cmake |& tee log
