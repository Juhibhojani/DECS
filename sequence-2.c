#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define N 5
// using an array of condition variables 
pthread_cond_t cond[N];
pthread_mutex_t lock;
int turn=0;

void initialize_CV_and_lock(){
    for(int i=0;i<N;i++){
        pthread_cond_init(&cond[i], NULL);
    }
    pthread_mutex_init(&lock,NULL);
    return;
}



void *print_thread_message(void *arg)
{
  int thread_num = *(int *)arg;
  while (1)
  {
    pthread_mutex_lock(&lock); 
    while(turn!=thread_num){
      pthread_cond_wait(&cond[thread_num], &lock);
    }
    printf("I am thread %d\n", thread_num);
    turn = (turn+1)%N;
    pthread_cond_signal(&cond[(thread_num+1)%N]);
    pthread_mutex_unlock(&lock);
    fflush(stdout);
  }
}

int main()
{
  pthread_t threads[N];
  //initialize all condition variable
  initialize_CV_and_lock();
  int thread_nums[N];
  for (int i = 0; i < N; i++)
    thread_nums[i] = i;
    // Create N threads
    for (int i = 0; i < N; i++)
      pthread_create(&threads[i], NULL, print_thread_message, &thread_nums[i]);

    // Wait for all threads (though in this case, the threads run forever)
    for (int i = 0; i < N; i++)
      pthread_join(threads[i], NULL);

  return 0;
}
