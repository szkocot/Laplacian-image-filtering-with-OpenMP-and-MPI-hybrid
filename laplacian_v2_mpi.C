#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>
#pragma warning(disable : 4996)
#pragma warning(disable : 4090)

// define 5x5 laplacian filter
const int laplacian[5][5] = {0, 0, 1, 0, 0, 0, 1, 2, 1, 0, 1, 2, -16, 2, 1, 0, 1, 2, 1, 0, 0, 0, 1, 0, 0};

void getFilename(char FileName[], int ncols, int nrows)
{

	char nrows_str[100];
	char ncols_str[100];

	sprintf(ncols_str, "%d", ncols);
	sprintf(nrows_str, "%d", nrows);

	strcat(FileName, ncols_str);
	strcat(FileName, "_");
	strcat(FileName, nrows_str);
	strcat(FileName, ".bin");
}

uint8_t conv(uint8_t *arr, int x, int y, int ncols, int nrows, const int filter[5][5])
{
	int i1, i2;
	uint8_t val = 0;
	for (i1 = 0; i1 < 5; i1++)
	{
		for (i2 = 0; i2 < 5; i2++)
		{
			val = val + (filter[i1][i2] * arr[(x + i1 - 2) + ncols * (y + i2 - 2)]);
		}
	}
	return val;
}

int main(int argc, char *argv[])
{
	int x, y;
	long int i;
	double start, end, cpu_time_used;

	// argv to nrows, ncols
	int ncols = atoi(argv[1]);
	int nrows = atoi(argv[2]);

	char inFileName[100] = "infile";
	char outFileName[100] = "outfile";

	// MPI
	MPI_Status status;
	MPI_Init(&argc, &argv);
	int myrank, numnodes;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &numnodes);

	int omp_chunk_size = 100;
	int mpi_chunks = numnodes;
	int mpi_chunk_size = (ncols * nrows) / mpi_chunks;

	// preallocate 2d array into 1d
	uint8_t *arr = (uint8_t *)malloc(nrows * ncols * sizeof(uint8_t));
	/*
	i = x + ncols*y;
	x = i % ncols;
	y = i / ncols;
	*/

	// subvector for gather
	uint8_t *temp = (uint8_t *)malloc(mpi_chunk_size * sizeof(uint8_t));

	if (myrank == 0)
	{
		// parse filename
		getFilename(inFileName, ncols, nrows);
		getFilename(outFileName, ncols, nrows);

		// read file
		printf("Reading  %s\n", inFileName);
		FILE *inputFile = fopen(inFileName, "rb");
		size_t count = fread(arr, sizeof(uint8_t), nrows * ncols, inputFile);
		fclose(inputFile);
	}

	// MPI_Bcast
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(arr, nrows * ncols, MPI_UINT8_T, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	if (myrank == 0)
	{
		// start clock
		start = MPI_Wtime();
	}

// convolve
#pragma omp parallel for private(x, y) schedule(dynamic, omp_chunk_size)
	for (i = mpi_chunk_size * myrank; i < mpi_chunk_size * (myrank + 1); i++)
	{
		x = i % ncols;
		y = i / ncols;
		if (x > 1 && y > 1 && x < (ncols - 2) && y < (nrows - 2))
		{
			// new calculated val
			temp[i - mpi_chunk_size * myrank] = conv(arr, x, y, ncols, nrows, laplacian);
		}
	}

	// MPI_Gather
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Gather(temp, mpi_chunk_size, MPI_UINT8_T, arr, mpi_chunk_size, MPI_UINT8_T, 0,
			   MPI_COMM_WORLD);

	// Summary
	if (myrank == 0)
	{
		// stop clock
		end = MPI_Wtime();
		cpu_time_used = (double)(end - start);
		printf("Filtration took %f seconds to execute \n", cpu_time_used);

		// write file
		FILE *outputFile = fopen(outFileName, "wb");
		fwrite(arr, sizeof(uint8_t), nrows * ncols, outputFile);
		printf("Writing  %s\n", outFileName);
		fclose(outputFile);

		printf("Success!\n");
	}
	// free res
	free(arr);
	free(temp);
	MPI_Finalize();
	return 0;
}
