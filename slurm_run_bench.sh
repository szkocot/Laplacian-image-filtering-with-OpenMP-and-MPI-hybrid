#!/bin/bash
#SBATCH --job-name=SK_MPI
#SBATCH -p ibm_large
#SBATCH --output=out.txt

module load mpi/openmpi/3.0.0
mpicc -xc laplacian_v1.C -fopenmp -o lp_v1 -O3 -march=native
chmod 755 lp_v1
cd ~/labMPI

export OMP_NUM_THREADS=1
echo 1
./lp_v1 45000 36000 100

for i in {2..48..2}
do
    export OMP_NUM_THREADS=$i
    echo $i
    ./lp_v1 45000 36000 100
done
