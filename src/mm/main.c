/*
 * Copyright(C) 2016 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of the LaPeSD Benchmarks Suite.
 * 
 * LaPeSD Benchmarks Suite is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * LaPeSD Benchmarks Suite is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with CAP Benchmarks. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <omp.h>

#include <profile.h>

#define NITERATIONS 5

/**
 * @param Sparse factor.
 */
#define SPARSE_FACTOR 80

#define MATRIX(m, i, j) m[(i)*n + (j)]

/**
 * @brief Performs a matrix multiplication (outer for).
 *
 * @param c Resulting matrix.
 * @param a First operand matrix.
 * @param b Second operand matrix.
 * @param n Matrix size.
 */
static void matrix_mult1(double *restrict c, const double *restrict a, const double *restrict b, int n)
{

	#pragma omp parallel
	{
		profile_start();

		#pragma omp for
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
			{
				for (int k = 0; k < n; k++)
					MATRIX(c, i, j) += MATRIX(a, i, k)*MATRIX(b, k, j);
			}
		}

		profile_end();
	}

	profile_dump();
}

/**
 * @brief Performs a matrix multiplication (inner for).
 *
 * @param c Resulting matrix.
 * @param a First operand matrix.
 * @param b Second operand matrix.
 * @param n Matrix size.
 */
static void matrix_mult2(double *restrict c, const double *restrict a, const double *restrict b, int n)
{
	for (int i = 0; i < n; i++)
	{
		#pragma omp parallel
		{
			profile_start();

			#pragma omp for
			for (int j = 0; j < n; j++)
			{
				for (int k = 0; k < n; k++)
					MATRIX(c, i, j) += MATRIX(a, i, k)*MATRIX(b, k, j);
			}

			profile_end();
		}
	}

	profile_dump();
}

/**
 * @brief Performs a sparse matrix multiplication
 *
 * @param c Resulting matrix.
 * @param a First operand (sparse) matrix.
 * @param b Second operand matrix.
 * @param n Matrix size.
 */
static void sparsematrix_mult(double *restrict c, const double *restrict a, const double *restrict b, int n)
{
	#pragma omp parallel
	{
		profile_start();
	#if defined(_SCHEDULE_DYNAMIC_)
		#pragma omp for schedule(dynamic)
	#elif defined(_SCHEDULE_GUIDED_)
		#pragma omp for schedule(guided)
	#elif defined (_SCHEDULE_STATIC_)
		#pragma omp for schedule(static)
	#else
		#error "no scheduling strategy defined"
	#endif
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
			{
				for (int k = 0; k < n; k++)
				{
					if (MATRIX(a, i, k) != 0)
						MATRIX(c, i, j) += MATRIX(a, i, k)*MATRIX(b, k, j);
				}
			}
		}
		profile_end();
	}

	profile_dump();
}

/**
 * @brief Prints program usage and exits.
 */
static void usage(void)
{
	printf("usage: mm <matrix size>\n");

	exit(EXIT_SUCCESS);
}

/**
 * @brief Creates a matrix.
 *
 * @param m Target matrix.
 */
static double *matrix_create(int n)
{
	double *m;

	m = malloc(n*n*sizeof(double));
	assert(m != NULL); 

	memset(m, 0, n*n*sizeof(double));

	return (m);
}

static void matrix_init(double *m, int n)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
			MATRIX(m, i, j) = rand()/10.0;
	}
}

/**
 * @brief Initializes a sparse matrix.
 *
 * @param m Target matrix.
 */
static double *sparsematrix_create(int n)
{	
	double *m;

	m = malloc(n*n*sizeof(double));
	assert(m != NULL); 

	memset(m, 0, n*n*sizeof(double));

	return (m);	
}

static void sparsematrix_init(double *m, int n)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			int zero = 0;

			if (i > n/2)
			{
				if ((rand()%100) <= SPARSE_FACTOR)
				   zero = 1;
			}
			
			MATRIX(m, i, j) = (zero) ? 0.0 : rand()/10.0;
		}
	}
}

int main(int argc, char **argv)
{
	int n;
	double *a1;
	double *a2;
	double *b;
	double *c1;
	double *c2;
	double *c3;
	double start, end;

	if (argc < 2)
		usage();
	
	/* Read command line arguments. */
	n = atoi(argv[1]);

	/* Sanity check. */
	assert(n > 0);

	/* Setup profiling. */
	profile_setup(omp_get_num_procs());

	a1 = matrix_create(n); matrix_init(a1, n);
	b  = matrix_create(n); matrix_init(b,  n);
	c1 = matrix_create(n); matrix_init(c1, n);
	c2 = matrix_create(n); matrix_init(c2, n);
	c3 = matrix_create(n); matrix_init(c3, n);
	a2 = sparsematrix_create(n); sparsematrix_init(a2, n);
	
	/* Benchmark 1. */
	for (int it = 0; it <= NITERATIONS; it++)
	{
		start = omp_get_wtime();
		matrix_mult1(c1, a1, b, n);
		end = omp_get_wtime();
		if (it)
			printf("mult1: %lf\n", end - start);
	}
	
	/* Benchmark 2. */
	for (int it = 0; it <= NITERATIONS; it++)
	{
		start = omp_get_wtime();
		matrix_mult2(c2, a1, b, n);
		end = omp_get_wtime();
		if (it)
			printf("mult2: %lf\n", end - start);
	}
	
	/* Benchmark 3. */
	for (int it = 0; it <= NITERATIONS; it++)
	{
		start = omp_get_wtime();
		sparsematrix_mult(c3, a2, b, n);
		end = omp_get_wtime();
		if (it)
			printf("sparsemult: %lf\n", end - start);
	}
	
	/* House keeping. */
	free(a1);
	free(a2);
	free(b);
	free(c1);
	free(c2);
	free(c3);

	return (EXIT_FAILURE);
}
