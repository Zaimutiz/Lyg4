#!/bin/sh
#SBATCH -p short
#SBATCH -n64
#SBATCH -C beta
#SBATCH -o MPI_DYNAMIC_RESULTS
#SBATCH -e MPI_DYNAMIC_ERRORS
mpic++ -pthread -o PrimeSieve PrimeSieve.cpp
mpirun PrimeSieve 
