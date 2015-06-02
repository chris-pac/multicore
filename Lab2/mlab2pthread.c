//
//  mlab2pthread.c
//  mlab2pthread
//
//  Created by Chris Pac on 10/28/14.
//  Copyright (c) 2014 Chris Pac. All rights reserved.
//

//  This is the pthreads version
//  gcc -o plab mlab2pthread.c -lpthread

#define MAX_THREADS 4

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

int u,r;

int *A;
int *B;
float *C;

typedef struct {
    float nMax, nMin;
    double sum;
    pthread_mutex_t m;
} results_t;

results_t m_myResults;

void* do_parallel(void *idx)
{
    int i = (int) idx;
    
    // load balance
    int s = i * u + (i > r ? r : i);
    int e = s + u + ((i+1) > r ? 0 : 1);
    int c = e-s;

    if (c)
    {
    	double sum = 0.0;
    	float nMax = 0.0;
    	float nMin = 10.0;
    	unsigned int rseed = time(NULL) - (i*11);
	for (; s < e; s++)
    	{
        	A[s] = rand_r(&rseed) % 10;
        	B[s] = rand_r(&rseed) % 10;
        	C[s] = (A[s]+B[s])/2;
        
        	sum += C[s];
        
        	if (C[s] > nMax)
            		nMax = C[s];
        	if (C[s] < nMin)
            		nMin = C[s];
    	}
    
        pthread_mutex_lock(&m_myResults.m);
        
        m_myResults.sum += sum;
        
        if (nMax > m_myResults.nMax)
            m_myResults.nMax = nMax;
        
        if (nMin < m_myResults.nMin)
            m_myResults.nMin = nMin;
        
        pthread_mutex_unlock(&m_myResults.m);
    }
    return NULL;
}

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
    
    m_myResults.nMax = 0.0;
    m_myResults.nMin = 10.0;
    m_myResults.sum = 0.0;
    pthread_mutex_init(&m_myResults.m, NULL);
    
    A = (int *) malloc(n * sizeof(int));
    B = (int *) malloc(n * sizeof(int));;
    C = (float *) malloc(n * sizeof(float));
    
    pthread_t T[thread_c];
    u = n/thread_c;
    r = n%thread_c;
    
    int i;
    for (i = 0; i < thread_c; i++)
    {
        pthread_create(&(T[i]), NULL, do_parallel, (void*)i);
    }
    
    for (i = 0; i < thread_c; i++)
    {
        pthread_join(T[i], NULL);
    }
    
    printf("pthreads - Average is: %.4f, Maximum is: %.2f, Minimum is: %.2f, Count: %d, Threads: %d\n", m_myResults.sum/n, m_myResults.nMax, m_myResults.nMin, n, thread_c);
    
    pthread_mutex_destroy(&m_myResults.m);
    free(A);
    free(B);
    free(C);
    
    return 0;
}
