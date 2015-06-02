//
//  mlab2openmp.c
//  mlab2openmp
//
//  Created by Chris Pac on 10/29/14.
//  Copyright (c) 2014 Chris Pac. All rights reserved.
//

//  This is the openmp version
//  gcc -fopenmp -o olab mlab2openmp.c

#define MAX_THREADS 4

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

int *A;
int *B;
float *C;

int main(int argc, const char * argv[]) {
    int n = 0;
    int thread_c = MAX_THREADS;
    
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) thread_c = atoi(argv[2]);
    
    if (!n || n < 0)
    {
        printf("Missing or invalid array size.\n");
        return 0;
    }
    
	omp_set_num_threads(thread_c);
    
    A = (int *) malloc(n * sizeof(int));
    B = (int *) malloc(n * sizeof(int));;
    C = (float *) malloc(n * sizeof(float));
    
	float nMaxT = 0.0;
	float nMinT = 10.0;
	double sumT = 0.0;

	#pragma omp parallel
	{
		int i;
		unsigned int rseed = time(NULL) - (omp_get_thread_num()*11);
		double sum = 0.0;
		float nMax = 0.0;
		float nMin = 10.0;
		#pragma omp for nowait schedule(static)
		for (i = 0; i < n; i++)
		{
			A[i] = rand_r(&rseed) % 10;
			B[i] = rand_r(&rseed) % 10;
			C[i] = (A[i]+B[i])/2;

			sum += C[i];
				
			if (C[i] > nMax)
				nMax = C[i];
			if (C[i] < nMin)
				nMin = C[i];
		}

		// implicit flush is done at entry/exit to critical region
		#pragma omp critical
		{
			sumT += sum;
			if (nMax > nMaxT)
				nMaxT = nMax;
			if (nMin < nMinT)
				nMinT = nMin;
		}		
	}

    printf("openmp - Average is: %.4f, Maximum is: %.2f, Minimum is: %.2f, Count: %d, Threads: %d\n", sumT/n, nMaxT, nMinT, n, thread_c);
    
    free(A);
    free(B);
    free(C);
    
    return 0;
}
