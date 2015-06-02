//
//  main.c
//  mlab1v3
//
//  Created by Chris Pac on 10/10/14.
//  Copyright (c) 2014 Chris Pac. All rights reserved.
//


//  CSCI-GA.3033-009
//  Multicore Processors: Architecture & Programming
//  Lab Assignment #1
//  Christopher Pac

/*
 *  The code is divided into sequential and parallel parts.
 *  Depending if MULTI is predefined as a macro at compile time
 *  will result in either sequential or parallel program.
 *  (default is sequential - i.e. no MULTI option specified).
 */

//  gcc -static -m32 -o slab1 main.c                        => for the sequential version
//  gcc -static -m32 -D MULTI -o mlab1 main.c -lpthread     => for the parallel version

/* INPUT:
 * n => loop count
 * t => thread count
 *
 *
 * (mlab n t)   => for the parallel version
 * (slab n)     => for the sequential version
 */


// keep this 'on' for testing parallel version
//#define MULTI

#include <stdio.h>
#include <stdlib.h>

#ifdef MULTI
#include <pthread.h>

pthread_t *ptrT;
double *ptrSum;
int u,r,t;


void* do_parallel(void *w);
#else
void do_sequential(int n);
#endif

int main(int argc, const char * argv[]) {
    // input n
    int n = 0;
    
    if (argc > 1) n = atoi(argv[1]);
    
    if (!n)
    {
        printf("Missing loop count n\n");
        return 0;
    }
    
#ifdef MULTI
    t = 0;
    if (argc > 2) t = atoi(argv[2]);
    
    if (t)
    {
        printf("Parallel Execution: loop count n=%d, thread count t=%d\n", n,t);
        /* Do Parallel Work */
        ptrT = (pthread_t *) malloc(t * sizeof(pthread_t));
        ptrSum = (double *) malloc(t * sizeof(double));
        u = n/t;
        r = n%t;
        
        // create threads in reverse order
        int i;
        for (i = t-1; 0 < i; i--)
        {
            pthread_create(&ptrT[i], NULL, do_parallel, (void*)i);
        }
        
        do_parallel((void*)0);
        
        printf("pi = % .15f\n", 4*ptrSum[0]);
        
        // no need to free mem since the proc is done
        // if the process was not finished at this point we would need to free
    }
    else
        printf("Missing Thread count\n");
#else
    printf("Sequential Execution: loop count n=%d\n", n);
    /* Do Sequential Work */
    do_sequential(n);
#endif
    
    return 0;
}

#ifdef MULTI
// Parallel Work START

void* do_parallel(void *w)
{
    int i = (int) w;
    
    //printf("thread %d is created\n", i);
    
    // load balance
    int s = i * u + (i > r ? r : i);
    int e = s + u + ((i+1) > r ? 0 : 1);
    
    // set factor based on odd or even value of s
    double factor = s & 1 ? -1.0 : 1.0;
    double sum = 0.0;
    
    //printf("t=%d: s=%d,e=%d,f=%.1f\n",i,s,e,factor);
    for (; s < e; s++)
    {
        sum += factor/(2*s + 1);
        factor = -factor;
    }
    
    ptrSum[i] = sum;
    
    // join the threads
    int v = 1;
    int join_t = 1;
    v = v << 1;
    
    // join the threads in the radix sum pattern as shown in class slides
    while(!(i%v))
    {
        join_t = join_t + i;
        //printf("thread %d is joining with thread %d\n", i,join_t);
        
        pthread_join(ptrT[join_t], NULL);
        sum += ptrSum[join_t];
        
        join_t = v;
        v = v << 1;
        // this is only required for thread 0
        if (join_t == t)
            break;
    }
    
    ptrSum[i] = sum;
    
    return NULL;
}

// Parallel Work END
#else
void do_sequential(int n)
{
    double factor = 1.0;
    double sum = 0.0;
    int k;
    for (k = 0; k < n; k++)
    {
        sum += factor/(2*k + 1);
        factor = -factor;
    }
    
    double pi_approx = 4.0*sum;
    
    printf("pi = % .15f\n", pi_approx);
}
#endif

