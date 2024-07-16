#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include<pthread.h>
#include<ctime>
#include <chrono>


struct thread_args{

    int low;
    int high;
    std::function<void(int)> func;

};

struct thread_args_2{

    int low;
    int high;
    int n;
    int m;
    std::function<void(int , int)> func;

};


void *run_lambda(void *ptr){
    thread_args *t = (thread_args*)ptr;

    int low =  t->low;
    int high = t->high;
    std::function<void(int)> lambda = t->func;


    for (int k = low; k < high; k++)
    {
        lambda(k);
    }

    return NULL;
    
}

void *run_lambda_2(void *ptr){
    thread_args_2 *t = (thread_args_2*)ptr;

    int low =  t->low;
    int high = t->high;
    int n = t->n;
    int m = t->m;
    std::function<void(int ,int)> lambda = t->func;

    int i,j;

    for (int k = low; k < high; k++)
    {
        j = k % n;
        i = (k-j)/n;
        lambda(i,j);
    }

    return NULL;
    
}


void parallel_for(int low, int high, std::function<void(int)> &&lambda , int numThreads){ 
    if(numThreads == 0){perror("invalid number of threads") ; exit(0);}

    if(!lambda) {perror("lambda functions") ; exit(0);}
    
    auto start = std::chrono::high_resolution_clock::now();
    int chunk = (high - low)/numThreads; 

    pthread_t tid[numThreads];
    thread_args args[numThreads];

    

    for (int i = 0; i < numThreads; i++)
    {
        
        args[i].low = i*chunk;
        args[i].high = (i+1)*chunk;
        args[i].func = lambda;

        if(i == numThreads -1) {args[i].high = high;}

        int check = pthread_create(  &tid[i] , NULL , run_lambda , (void*) &args[i]  );
        if(check == -1){
            perror("pthread_create");
        }
        
    }

    for (int i = 0; i < numThreads; i++)
    {
        int check = pthread_join(tid[i] , NULL);
        if(check == -1){
            perror("pthread_join");
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto tm = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printf("Time taken for the for_loop: %ld\n" ,tm.count());


}


void parallel_for(int low1, int high1, int low2, int high2,std::function<void(int, int)> &&lambda, int numThreads){
    if(numThreads == 0){perror("invalid number of threads") ; exit(0);}
    if(!lambda) {perror("lambda functions") ; exit(0);}

    auto start = std::chrono::high_resolution_clock::now();
    int chunk = (high1 - low1)*(high2 -low2)/numThreads; 
    pthread_t tid[numThreads];
    thread_args_2 args[numThreads];
    

    for (int i = 0; i < numThreads; i++)
    {
        
        args[i].low = i*chunk;
        args[i].high = (i+1)*chunk;
        args[i].func = lambda;
        args[i].n = high1 -low1;
        args[i].m = high2 -low2;

        if(i == numThreads -1) {args[i].high =(high1 - low1)*(high2 -low2);}

        int check = pthread_create(  &tid[i] , NULL , run_lambda_2 , (void*) &args[i]  ); 
        if(check == -1){
            perror("pthread_create");
        }
    }

    for (int i = 0; i < numThreads; i++)
    {
        int check = pthread_join(tid[i] , NULL);
        if(check == -1){
            perror("pthread_join");
        }
    }
  
    auto end = std::chrono::high_resolution_clock::now();
    auto tm = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    printf("Time taken for the for_loop: %ld\n" ,tm.count());
}





