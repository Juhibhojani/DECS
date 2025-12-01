#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// solution using single condition variable and wake up all threads
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int to_execute = 0;

#define N 5

void *print_thread_message(void *arg)
{
  int thread_num = *(int *)arg;
  while (1)
  {
    pthread_mutex_lock(&lock); 
    while(to_execute!=thread_num){
      pthread_cond_wait(&cond, &lock);
    }
    printf("I am thread %d\n", thread_num);
    to_execute=(thread_num+1)%N;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
    fflush(stdout);
  }
}

int main()
{

  pthread_t threads[N];
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
