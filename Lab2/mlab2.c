//
//  mlab2.c
//  mlab2
//
//  Created by Chris Pac on 10/28/14.
//  Copyright (c) 2014 Chris Pac. All rights reserved.
//

//  This is the sequential version
//  gcc -o slab mlab2.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int *A;
int *B;
float *C;

int main(int argc, const char * argv[]) {
    int n = 0;
    
    if (argc > 1) n = atoi(argv[1]);
    
    if (!n || n < 0)
    {
        printf("Missing or invalid array size.\n");
        return 0;
    }
    
    srand (time(NULL));
    
    A = (int *) malloc(n * sizeof(int));
    B = (int *) malloc(n * sizeof(int));;
    C = (float *) malloc(n * sizeof(float));
    
    int i;
    double sum = 0.0;
    float nMax = 0.0;
    float nMin = 10.0;
    for (i = 0; i < n; i++)
    {
        A[i] = rand() % 10;
        B[i] = rand() % 10;
        C[i] = (A[i]+B[i])/2;
        
        sum += C[i];
        
        if (C[i] > nMax)
            nMax = C[i];
        if (C[i] < nMin)
            nMin = C[i];
    }
    
    printf("Average is: %.4f, Maximum is: %.2f, Minimum is: %.2f, Count: %d\n", sum/n, nMax, nMin, n);
    
    free(A);
    free(B);
    free(C);
    
    return 0;
}
