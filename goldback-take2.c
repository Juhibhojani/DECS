#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>

#define maxN 100000
// defining them as global variables so can be used by all threads
bool primes[maxN + 1];
int gb_count[maxN / 2 + 1];
// pthread_mutex_t lock;

// structure to pass as arguement to function
typedef struct
{
    int start;
    int end;
} myarg_t;

typedef struct
{
    int gb_count_values[maxN + 1];
} myret_t;

// fill the value of primes
int fill_primes()
{
    // Sieve of Eratosthenes
    for (int i = 0; i <= maxN; i++)
    {
        primes[i] = true;
    }
    for (int i = 0; i <= maxN / 2; i++)
    {
        gb_count[i] = 0;
    }

    primes[0] = primes[1] = false;

    for (int p = 2; p <= sqrt(maxN); p++)
    {
        if (primes[p])
        {
            for (int i = p * p; i <= maxN; i += p)
                primes[i] = false;
        }
    }

    int prime_count = 0;
    for (int p = 2; p <= maxN; p++)
        if (primes[p])
        {
            prime_count++;
        }

    return prime_count;
}

void *compute_goldback(void *args)
{
    myarg_t *arg = (myarg_t *)args;
    int start = arg->start;
    int end = arg->end;
    // defining function to return on heap as heap is shared between processes
    myret_t *rvals = calloc(1, sizeof(myret_t));
    if (start == 0)
    {
        start = 4;
    }
    if (end == 99999)
    {
        end = 100000;
    }
    printf("some thread started with values of start %d and end %d \n", start, end);
    for (int n = start; n <= end; n += 2)
    {
        for (int p1 = 2; p1 <= n / 2; p1++)
        {
            int p2 = n - p1;
            if (primes[p1] && primes[p2])
            {
                // it will modify it's own local variable
                rvals->gb_count_values[n / 2]++;
            }
        }
    }
    return (void *) rvals;
}

void main(int argc, char *argv[])
{
    int prime_count = fill_primes();
    printf("Computed primes upto %d, count = %d\n", maxN, prime_count);

    // Compute number of Goldbach pairs

    // start timer
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, 0);

    // create 10 threads
    pthread_t p[10];
    myarg_t args[10];
    int already_computed = -1;
    //   int rc = pthread_mutex_init(&lock, NULL); // lock needed to update the value of gb_count, no need of lock for primes as we are just reading it
    //   assert(rc == 0); // always check success!

    // THE CALCULATIONS BELOW TO BE DONE IN PARALLEL
    for (int i = 0; i < 10; i++)
    {
        args[i].start = already_computed + 1;
        args[i].end = args[i].start + (maxN / 10) - 1;
        // create pthread
        int rc = pthread_create(&p[i], NULL, compute_goldback, &args[i]);
        assert(rc == 0); // always check success
        already_computed = already_computed + 1 + (maxN / 10) - 1;
    }

    // THREADS TO JOIN HERE, DO NOT CHANGE CODE BELOW
    for (int i = 0; i < 10; i++)
    {
        int start = args[i].start;
        int end = args[i].end;
        myret_t *rvals;
        pthread_join(p[i], (void **)&rvals);
        if (start == 0)
        {
            start = 4;
        }
        if (end == 99999)
        {
            end = 100000;
        }
        // now store this into the original array
        for (int n = start; n <= end; n += 2){
            gb_count[n/2]=rvals->gb_count_values[n / 2];
        }
        free(rvals);
    }

    // end timer and calculate elapsed time
    gettimeofday(&tv_end, 0);
    unsigned long elapsed_usec = ((tv_end.tv_sec * 1000000) + tv_end.tv_usec) - ((tv_start.tv_sec * 1000000) + tv_start.tv_usec);
    printf("elapsed time: %lu microseconds\n", elapsed_usec);

    // print values to file for checking correctness
    FILE *fptr = fopen("output-take-2.txt", "w");
    for (int i = 4; i <= maxN; i += 2)
        fprintf(fptr, "%d %d\n", i, gb_count[i / 2]);
    fclose(fptr);
}
