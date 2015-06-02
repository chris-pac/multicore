//
//  finalBaB.cpp
//  TSPBaBv2
//
//  Created by Chris Pac on 11/21/14.
//  Copyright (c) 2014 Chris Pac. All rights reserved.
//

#include <iostream>
#include <queue>
#include <stack>
#include <pthread.h>
#include <string.h>
#include <limits.h>

using namespace std;

//#define PRINT_T_LOAD

#ifndef MY_INIT_THREADS
#define MY_INIT_THREADS 4
#endif

#ifndef MAX_CITIES
#define MAX_CITIES 10
#endif
int totalThreads;

int V;
int *graph;

typedef unsigned char path;

pthread_mutex_t sync_m;
pthread_cond_t sync_c;
short sync_count;
int global_minW;
path globalFinalPath[MAX_CITIES];

static const int MultiplyDeBruijnBitPosition2[32] =
{
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

void init_tsp_globalPathAndWeight()
{
    int idx = 1;
    int min_w = INT_MAX;
    int nid = 0;
    int w = 0;
    int set = ((1<<V) - 1) ^ 1;
    
    globalFinalPath[0] = 0;
    global_minW = 0;
    
    int itr = set;
    
    while (itr)
    {
        nid = (itr & ~(itr-1));
        itr = itr ^ nid;
        
        nid = MultiplyDeBruijnBitPosition2[(uint32_t)(nid * 0x077CB531U) >> 27];
        
        w = graph[V*globalFinalPath[idx-1] + nid];
        
        if (w < min_w)
        {
            min_w = w;
            globalFinalPath[idx] = nid;
        }
        
        if (itr == 0)
        {
            global_minW += min_w;
            
            set = set ^ (1 << globalFinalPath[idx]);
            itr = set;
            idx++;
            
            min_w = INT_MAX;
        }
    }
}

class tsp_node
{
public:
    path visited2[MAX_CITIES];
    int id;
    int set;
    int w;
    
    tsp_node();
    void init_node(tsp_node *parentNode, int myid);
    
    bool IsLeaf();
    int GetNextChild(tsp_node &child, int itr);
};

int tsp_node::GetNextChild(tsp_node &child, int itr)
{
    int cid = (itr & ~(itr-1));
    child.init_node(this, cid);
    
    return itr ^ cid;
}

tsp_node::tsp_node()
{
    id = 0;
    w = 0;
    memset(visited2, 0, sizeof(visited2));
    
    set = ((1<<V) - 1) ^ 1;
}

void tsp_node::init_node(tsp_node *parentNode, int myid)
{
    memcpy(visited2, parentNode->visited2, sizeof(visited2));

    set = parentNode->set;
    w = parentNode->w;
    
    set = set ^ myid;
    id = MultiplyDeBruijnBitPosition2[(uint32_t)(myid * 0x077CB531U) >> 27];
    
    w = w + graph[V*parentNode->id + id];
    
    
    visited2[0] = (int)visited2[0] + 1;
    visited2[visited2[0]] = id;
}

bool tsp_node::IsLeaf()
{
    return set == 0;
}

class ThreadStackTSP
{
    // make it two stacks in_s and out_s then swap
protected:
    pthread_mutex_t m;
    std::stack<tsp_node> s;
    
public:
    ThreadStackTSP();
    ~ThreadStackTSP();
    void push(tsp_node);
    bool pop(tsp_node&);
};

ThreadStackTSP::ThreadStackTSP()
{
    pthread_mutex_init(&m, NULL);
}

ThreadStackTSP::~ThreadStackTSP()
{
    pthread_mutex_destroy(&m);
}

bool ThreadStackTSP::pop(tsp_node& node)
{
    bool ret = false;
    pthread_mutex_lock(&m);
    if (!s.empty())
    {
        node = s.top();
        s.pop();
        ret = true;
    }
    pthread_mutex_unlock(&m);
    
    return ret;
}

void ThreadStackTSP::push(tsp_node node)
{
    pthread_mutex_lock(&m);
    s.push(node);
    pthread_mutex_unlock(&m);
}

void tsp_BaB_seq()
{
    //queue<tsp_node> leftQ;
    stack<tsp_node> leftQ;
    
    tsp_node node, child;
    int itr = 0;
    
    int minW = INT_MAX;
    
    leftQ.push(node);
    
    while (!leftQ.empty())
    {
        //node = leftQ.front();
        node = leftQ.top();
        leftQ.pop();
        
        if (node.IsLeaf())
        {
            if (node.w < minW)
            {
                minW = node.w;
                global_minW = minW;
                memcpy(globalFinalPath, node.visited2, sizeof(globalFinalPath));
            }
        }
        else
        {
            itr = node.set;
            while (itr)
            {
                itr = node.GetNextChild(child, itr);
                if (child.w < minW)
                    leftQ.push(child);
            }
        }
    }
}

ThreadStackTSP *tsp_stack;

void *tsp_BaB_par(void *idx)
{
    //queue<tsp_node> leftQ;
    stack<tsp_node> leftQ;
    
    long tid = (long) idx;
    
    tsp_node node, child;
    int itr = 0;
    bool added = false;
   
    #ifdef PRINT_T_LOAD 
    int workdone=0;
    #endif
    
    pthread_mutex_lock(&sync_m);
    sync_count--;
    pthread_mutex_unlock(&sync_m);
    
    if (tid == 0)
    {
        leftQ.push(node);
        pthread_mutex_lock(&sync_m);
        totalThreads--;
        pthread_mutex_unlock(&sync_m);
    }
    
    while (true)
    {
        if (!leftQ.empty())
        {
            //node = leftQ.front();
            node = leftQ.top();
            leftQ.pop();
        }
        else if (!tsp_stack->pop(node))
        {
            pthread_mutex_lock(&sync_m);
            sync_count++;
            
            pthread_cond_broadcast(&sync_c);
            while(!tsp_stack->pop(node) && sync_count != totalThreads)
            {
                pthread_cond_wait(&sync_c, &sync_m);
            }
            
            if (sync_count == totalThreads)
            {
                #ifdef PRINT_T_LOAD
                printf("Thread: %d has done this much work %d\n", tid, workdone);
                #endif
                pthread_mutex_unlock(&sync_m);
                return NULL;
            }
            
            sync_count--;
            pthread_mutex_unlock(&sync_m);
        }
        
        if (node.IsLeaf())
        {
            pthread_mutex_lock(&sync_m);
            if (node.w < global_minW)
            {
                global_minW = node.w;
                memcpy(globalFinalPath, node.visited2, sizeof(globalFinalPath));
            }
            pthread_mutex_unlock(&sync_m);
        }
        else
        {
            added = false;
            if (node.w > global_minW)
            {
                continue;
            }
           
            #ifdef PRINT_T_LOAD 
            workdone++;
            #endif
            
            itr = node.set;
            
            if (itr)
            {
                itr = node.GetNextChild(child, itr);
                if (child.w < global_minW)
                    leftQ.push(child);
            }
            
            while (itr)
            {
                itr = node.GetNextChild(child, itr);
                if (child.w < global_minW)
                {
                    if (sync_count)
                    {
                        tsp_stack->push(child);
                        added = true;
                    }
                    else
                        leftQ.push(child);
                }
            }
            
            if (added)
                pthread_cond_broadcast(&sync_c);
        }
    }
    
    return NULL;
}

int main(int argc, const char * argv[]) {
    FILE *myFile;
    if (argc < 1)
    {
        printf("Missing file\n");
        return 1;
    }
    else
        myFile = fopen(argv[1], "r");
    
    if (myFile == NULL)
    {
        printf("Error Reading File\n");
        return 1;
    }
    
    V = 0;
    int ch;
    while(!feof(myFile))
    {
        ch = fgetc(myFile);
        if(ch == '\n')
        {
            V++;
        }
    }
    
    rewind(myFile);
    
    graph = (int *) malloc(V * V * sizeof(int));
    
    int i = 0;
    while ( fscanf(myFile, "%d", &graph[i]) != EOF)
        i++;

    global_minW = INT_MAX;
    init_tsp_globalPathAndWeight();

    #if MY_INIT_THREADS == 0
    cout << "Serial" << endl;
    tsp_BaB_seq();
    #else
    cout << "Parallel" << endl;
    totalThreads = MY_INIT_THREADS;
    
    if (argc > 2) totalThreads = atoi(argv[2]);
    
    pthread_mutex_init(&sync_m, NULL);
    pthread_cond_init(&sync_c, NULL);
    
    totalThreads++; // plus one for main
    sync_count = totalThreads;
    
    tsp_stack = new ThreadStackTSP;
    pthread_t T[totalThreads-1];
    
    totalThreads++; // tmp increase to cause a barrier (hack)
    
    for (long j=1; j < totalThreads-1; j++)
    {
        pthread_create(&(T[j-1]), NULL, tsp_BaB_par, (void*)j);
    }
    
    tsp_BaB_par((void*)0);
    
    for (i = 0; i < totalThreads-1; i++)
    {
        pthread_join(T[i], NULL);
    }
    #endif

    globalFinalPath[0] = 0;
    printf("Best path: ");
    for (i=0; i < V; i++)
        printf("%d ",globalFinalPath[i]);
   
    printf("\nDistance: %d\n",global_minW);
    
    return 0;
}
