#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <string.h>
#include <omp.h>
#pragma warning(disable:4996)
#pragma warning(disable:4090)

// define 5x5 laplacian filter
const int laplacian[5][5] = { 0,0,1,0,0,0,1,2,1,0,1,2,-16,2,1,0,1,2,1,0,0,0,1,0,0 };


void getFilename(char FileName[], int ncols, int nrows) {

	char nrows_str[100];
	char ncols_str[100];

	sprintf(ncols_str, "%d", ncols);
	sprintf(nrows_str, "%d", nrows);

	strcat(FileName, ncols_str);
	strcat(FileName, "_");
	strcat(FileName, nrows_str);
	strcat(FileName, ".bin");
}

uint8_t conv(uint8_t *arr, int x, int y, int ncols, int nrows, const int filter[5][5]) {
	int i1, i2;
	uint8_t val = 0;
	for (i1 = 0; i1 < 5; i1++) {
		for (i2 = 0; i2 < 5; i2++) {
			val = val + (filter[i1][i2] * arr[(x + i1 - 2) + ncols * (y + i2 - 2)]);
		}
	}
	return val;
}

int main(int argc,char *argv[])
{
	double start, end;
	double cpu_time_used;
	int omp_chunk;

	// parse OMP chunksize if specified
	if (argc == 4) {
		omp_chunk = atoi(argv[3]);
	}
	else {
		omp_chunk = 100;
	}

	// argv to nrows, ncols
	int ncols = atoi(argv[1]);
	int nrows = atoi(argv[2]);

	// parse filename
	char inFileName[100] = "infile";
	char outFileName[100] = "outfile";
	getFilename(inFileName, ncols, nrows);
	getFilename(outFileName, ncols, nrows);

	// preallocate 2d array into 1d
	uint8_t *arr = (uint8_t *)malloc(nrows * ncols * sizeof(uint8_t));
	/*
	i = x + ncols*y;
	x = i % ncols;
	y = i / ncols;
	*/

	// read file
	printf("Reading  %s\n", inFileName);
	FILE *inputFile = fopen(inFileName, "rb");
	size_t count = fread(arr, sizeof(uint8_t), nrows * ncols, inputFile);
	fclose(inputFile);
	
	// copy array
	uint8_t *new_arr = (uint8_t *)malloc(nrows * ncols * sizeof(uint8_t));
	memcpy(new_arr, arr, nrows * ncols * sizeof(uint8_t));

	//start clock
	start = omp_get_wtime();
	
	// convolve
	long int i;
	int x, y;

	#pragma omp parallel for private(x,y) schedule (dynamic,omp_chunk)
	for (i = 0; i < ncols * nrows; i++) {
		x = i % ncols;
		y = i / ncols;
		if (x > 1 && y > 1 && x < (ncols - 2) && y < (nrows - 2)) {
			// new calculated val
			new_arr[i] = conv(arr, x, y, ncols, nrows, laplacian);
		}
	}

	// stop clock
	end = omp_get_wtime();
	cpu_time_used = (double)(end - start);
	printf("Filtration took %f seconds to execute \n", cpu_time_used);

	// write file
	FILE *outputFile = fopen(outFileName, "wb");
	fwrite(new_arr, sizeof(uint8_t), nrows * ncols, outputFile);
	printf("Writing  %s\n", outFileName);
	fclose(outputFile);

	// free res
	free(arr);
	free(new_arr);
	printf("Success!\n");

	return 0;
}
