#include <iostream>
#include <mpi.h>
#include <cmath>
#include <chrono>

#define N 100000000

using namespace std;

void Master(int rank, int comm_size);
void Slave(int rank, int comm_size);
void PrimeSieve(int upper_bound, int lower_bound, bool prime[], int rank, int comm_size);

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    //rank = thread_ID, comm_size = threadcount
    int rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    //the first thread goes to master function every other goes to slave
	if (rank == 0)
		Master(rank, comm_size);
	else
		Slave(rank, comm_size);

    MPI_Finalize();
}

void Master(int rank, int comm_size)
{
    //a size for an interval
	auto time_start = std::chrono::high_resolution_clock::now();

    int splice_count = comm_size - 1;
    int splices[splice_count];
    splices[0] = 0;
    for (int i = 1; i < splice_count; ++i)
        splices[i] = ceil((double)(N - splices[i - 1]) / (splice_count - i + 1)) + splices[i - 1];

    //array size
    bool *bool_arr= new bool[N];

    MPI_Status status;
    // its for every block to be sent to slave
    for (int block = 0; block < splice_count-1; block++)
    {
        int test = splices[block];
        int test1 = splices[block+1]-1;
        MPI_Send(&test, 1, MPI_INT, rank +block+ 1, 0, MPI_COMM_WORLD);
        MPI_Send(&test1, 1, MPI_INT, rank +block+ 1, 1, MPI_COMM_WORLD);

        //           message buffer??        size of the data item   destination  chosen flag
        //                                   interval    as integer  process thread_ID   always has to be ussed
    }

    int test = splices[splice_count-1];
    int test1 = N - 1;
    MPI_Send(&test, 1, MPI_INT, rank +splice_count, 0, MPI_COMM_WORLD);
    MPI_Send(&test1, 1, MPI_INT, rank +splice_count, 1, MPI_COMM_WORLD);

	for (int block = 0; block < splice_count-1; ++block)
	{
        //blocking test for a message???
		MPI_Probe(rank+block+1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		//        destination    any tag    communicator?    MPI status
		//      process thread_ID
		MPI_Recv(bool_arr + splices[block], splices[block+1]-splices[block]+1, MPI_C_BOOL, rank+block+1, 2, MPI_COMM_WORLD, &status);
		//        initial address of        size of this data item thread_ID   chosen always has   MPI status
		//        receive buffer??           interval   as integer             flag  to be used

	}

	MPI_Probe(splice_count, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    MPI_Recv(bool_arr + splices[splice_count-1], N-splices[splice_count-1], MPI_C_BOOL, rank+splice_count, 2, MPI_COMM_WORLD, &status);

	auto time_stop = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> time_exec = time_stop - time_start;
	std::cout << "Execution time: " << time_exec.count() << " ms\n";

    /*for (int i=2; i<=N; i++)
        if(bool_arr[i])
            cout << i << " ";*/

	delete[] bool_arr;
}

void Slave(int rank, int comm_size)
{
    MPI_Status status;
    int lower_bound, upper_bound;


	MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	//       destination    any tag    communicator?    MPI status
	//      process thread_ID

	MPI_Recv(&lower_bound, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

    //initial address. of size of. data item. thread_ID.   chosen. always has   MPI status
    //receive buffer? this interval. as integer.           flag.  to be used

    MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&upper_bound, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);

	int block_size = upper_bound - lower_bound + 1;
    bool *bool_arr = new bool[block_size];
	memset(bool_arr, true, block_size);

	PrimeSieve(upper_bound, lower_bound, bool_arr, rank, comm_size);

	MPI_Send(bool_arr, block_size, MPI_C_BOOL, 0, 2, MPI_COMM_WORLD);
	// message buffer??size of the. data item.   destination  chosen flag
    //                 interval   . as integer.  process thread_ID   always has to be ussed
    delete[] bool_arr;
}

void PrimeSieve(int upper_bound, int lower_bound, bool prime[], int rank, int comm_size)
{
	int current_prime = 2;
	int signal; // -1 (thread explored all primes within bounds) or the next prime
	int exploring_thread_rank = 1; // current thread that feeds primes to other threads
	while (true)
	{
		if (exploring_thread_rank == rank) { // feeding thread
			for (int i = current_prime; i <= upper_bound; i++)
			{
				if (prime[i-lower_bound] == true)
				{
					signal = i; // feed prime number to other threads
					//cout << "Rank " << rank << " : " << i << endl;
					for (int next_rank = rank + 1; next_rank < comm_size; next_rank++)
						MPI_Send(&signal, 1, MPI_INT, next_rank, 3, MPI_COMM_WORLD);

					if (i <= upper_bound / i) {
						for (int j = i * i; j <= upper_bound; j += i)
							prime[j - lower_bound] = false;
					}
				}
			}
			signal = -1; // send signal to alert other threads of this thread's job completion
			for (int next_rank = rank + 1; next_rank < comm_size; next_rank++)
				MPI_Send(&signal, 1, MPI_INT, next_rank, 3, MPI_COMM_WORLD);
			break;
		}
		else // leaching thread
		{
			MPI_Status status;
			int signal;
			MPI_Probe(exploring_thread_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			MPI_Recv(&signal, 1, MPI_INT, exploring_thread_rank, 3, MPI_COMM_WORLD, &status);
			if (signal == -1)
			{
				exploring_thread_rank++; // switch to the following thread as a feeding thread
				current_prime = lower_bound;
			}
			else
			{
				current_prime = signal;
				int k = ceil((double)lower_bound / current_prime);
				if (current_prime <= upper_bound / current_prime) {
					int j = current_prime * k;
					if (current_prime * current_prime > lower_bound)
						j = current_prime * current_prime;

					for (j; j <= upper_bound; j += current_prime)
						prime[j - lower_bound] = false;
				}
			}
		}
	}
}
