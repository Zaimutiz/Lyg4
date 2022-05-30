#!/bin/sh
#SBATCH -p short
#SBATCH -n64
#SBATCH -C beta
#SBATCH --time=2
#SBATCH -o MPI_DYNAMIC_RESULTS
#SBATCH -e MPI_DYNAMIC_ERRORS
mpic++ -pthread -o MPI_DYNAMIC PrimeSieve.cpp
mpirun MPI_DYNAMIC
