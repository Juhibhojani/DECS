#include<stdio.h>
#include<pthread.h>
#include<assert.h>

void * print_args_threads(void *args){
    int *val = (int*) args;
    printf("I am thread %d \n",*val);
    return NULL;
}

int main(){
    pthread_t p[10];
    int counter[10];
    // create 10 threads
    for(int i=0;i<10;i++){
        counter[i] = i+1;
        int rc = pthread_create(&p[i],NULL,print_args_threads,&counter[i]);
        assert(rc==0);
    }
    for(int i=0;i<10;i++){
        // wait for thread
        pthread_join(p[i],NULL);
    }
    printf("I am main thread\n");
    return 0;
}