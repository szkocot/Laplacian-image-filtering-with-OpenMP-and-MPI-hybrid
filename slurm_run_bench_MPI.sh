#!/bin/bash
#SBATCH --job-name=SK_MPI
#SBATCH -p ibm_large
#SBATCH --output=out.txt
#SBATCH --nodes=4
#SBATCH --contiguous

module load mpi/openmpi/3.0.0
mpicc -xc laplacian_v2_mpi.C -fopenmp -o lp_v2_mpi -O3 -march=native
chmod 755 lp_v2_mpi
cd ~/labMPI

export OMP_NUM_THREADS=24
export OMP_PLACES=cores
export KMP_AFFINITY=verbose

for i in {1..4}
do
    echo $i
    mpiexec $FLAGS_MPI_BATCH -np $i -x OMP_NUM_THREADS -x OMP_PLACES -x KMP_AFFINITY \
                            --map-by ppr:1:node --bind-to board lp_v2_mpi \
                            45000 36000
done
