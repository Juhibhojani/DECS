#include <stdio.h>
#include <pthread.h>
#include <assert.h>

pthread_mutex_t lock;
int counter = 0;

// remove locks to see incorrect execution of program

void *increment_counter_thread() {
    printf("Thread Id: %ld , Starting code for incrementing counter.\n",pthread_self());
    // if lock is placed at this position and removed at end of for loop then also it works but reduces concurrency
    for(int i=0;i<1000;i++){
        pthread_mutex_lock(&lock); // taking lock before entering critical section
        counter++;
        pthread_mutex_unlock(&lock); // unlock after leaving CS
    }
    printf("Thread ID: %ld, work done!\n",pthread_self());
    return NULL;
}

int main(int argc, char *argv[]) {
    printf("Thread ID(Default thread): %ld \n",pthread_self());
    // array containing 10 threads
    pthread_t p[10];
    // using locks for synchronization in threads
    // initialize the lock
    int rc = pthread_mutex_init(&lock, NULL);
    assert(rc == 0); // always check success!

    for(int i=0;i<10;i++){
        // creating threads
        int rc = pthread_create(&p[i], NULL, increment_counter_thread, NULL);
        assert(rc == 0); // always check success!

    }
    for(int i=0;i<10;i++){
        pthread_join(p[i],NULL);
    }
    pthread_mutex_destroy(&lock);
    printf("All threads execution completed! and value of counter: %d \n",counter);
}
