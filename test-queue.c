#define _GNU_SOURCE
#include <omp.h>
#include <sched.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <errno.h>
#include <stdatomic.h>

#ifdef LOCK
#include "queue-lock.h"
#elif defined(LOCKFREE)
#include "queue-lockfree.h"
#elif defined(CK)
#include "queue-lockfree-ck.h"
#else
#error "must define a queue implementation"
#endif

#define N_TEST1 1000000
#define N_TEST2 1000000
#define N_TEST3 500000

static int nthr = 0;
Q *q;

static struct timeval start_time;
static struct timeval end_time;

typedef struct {
    int data;
    char pad[60];
} padded_int;

static void calc_benchmark(struct timeval *start, struct timeval *end, int actions)
{
    if (end->tv_usec < start->tv_usec) 
    {
        end->tv_sec -= 1;
        end->tv_usec += 1000000;
    }

    assert(end->tv_sec >= start->tv_sec);
    assert(end->tv_usec >= start->tv_usec);
    struct timeval interval = {
        end->tv_sec - start->tv_sec,
        end->tv_usec - start->tv_usec
    };

    double interval_sec = interval.tv_sec + interval.tv_usec / 1000000.0;
    printf("%8.0f/s  %0.6fµs\t\t", actions / interval_sec, interval_sec / actions * 1000000.0);
}

void bind_core(int threadid) 
{
    int logical_id = (threadid * 2) % 64;

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(logical_id, &set);

    if (sched_setaffinity(0, sizeof(set), &set) != 0) 
    {
        perror("Set affinity failed");
        exit(EXIT_FAILURE);
    }
}

void test1()
{
    q = queue_init();

    #pragma omp parallel num_threads(nthr)
    {
        int id = omp_get_thread_num();
        bind_core(id);

        #pragma omp barrier
        #pragma omp master
        {
            gettimeofday(&start_time, NULL);
        }

        int partition_size = N_TEST1 / nthr;
        int start = id * partition_size + 1;
        int end = (id == nthr - 1) ? N_TEST1 : start + partition_size - 1;
        
        for (int i = start; i <= end; i++)
            enqueue(q, i);

        #pragma omp barrier
        #pragma omp master
        {
            gettimeofday(&end_time, NULL);
        }

        #if defined(LOCKFREE) || defined(CK)
        fl_free();
        #endif
    }

    calc_benchmark(&start_time, &end_time, N_TEST1);
    
    int *seen = calloc(N_TEST1 + 1, sizeof(int));
    int data;
    while (!dequeue(q, &data))
    {
        if (data < 1 || data > N_TEST1)
        {
            printf("invalid data %d in queue\n", data);
            exit(1);
        }
        seen[data]++;
    }

    for (int i = 1; i <= N_TEST1; i++)
    {
        if (seen[i] != 1)
        {
            printf("data %d was found %d times in queue\n", i, seen[i]);
            exit(1);
        }
    }

    free(seen);
    #if defined(LOCKFREE) || defined(CK)
    fl_free();
    #endif
    queue_free(q);
}

void test2()
{
    q = queue_init();
    padded_int *seen = calloc(N_TEST2 + 1, sizeof(padded_int));

    for (int i = 1; i <= N_TEST2; i++)
        enqueue(q, i);

    #pragma omp parallel num_threads(nthr)
    {
        int id = omp_get_thread_num();
        bind_core(id);

        #pragma omp barrier
        #pragma omp master
        {
            gettimeofday(&start_time, NULL);
        }

        int partition_size = N_TEST2 / nthr;
        
        int data;
        for (int i = 0; i < partition_size; i++)
        {
            if (!dequeue(q, &data)) seen[data].data++;
        }

        #pragma omp barrier
        #pragma omp master
        {
            gettimeofday(&end_time, NULL);
        }

        #if defined(LOCKFREE) || defined(CK)
        fl_free();
        #endif
    }

    calc_benchmark(&start_time, &end_time, N_TEST2);

    for (int i = 1; i <= N_TEST2; i++)
    {
        if (seen[i].data != 1)
        {
            printf("data %d was found %d times in queue\n", i, seen[i].data);
            exit(1);
        }
    }

    int data;
    if (!dequeue(q, &data)) 
    {
        printf("queue isn't empty\n");
        exit(1);
    }

    free(seen);
    queue_free(q);
}

void test3()
{
    q = queue_init();
    padded_int *seen = calloc(N_TEST3 + 1, sizeof(padded_int));

    #pragma omp parallel num_threads(nthr)
    {
        int id = omp_get_thread_num();
        bind_core(id);

        #pragma omp barrier
        #pragma omp master
        {
            gettimeofday(&start_time, NULL);
        }

        int partition_size = N_TEST3 / nthr;
        int start = id * partition_size + 1;
        int end = (id == nthr - 1) ? N_TEST3 : start + partition_size - 1;
        
        for (int i = start; i <= end; i++)
            enqueue(q, i);

        #pragma omp barrier
        #pragma omp master
        {
            gettimeofday(&end_time, NULL);
            calc_benchmark(&start_time, &end_time, N_TEST3);
            gettimeofday(&start_time, NULL);
        }

        partition_size = N_TEST3 / nthr;
        start = (id * partition_size + 1) + N_TEST3;
        end = (id == nthr - 1) ? N_TEST3 * 2 : start + partition_size - 1;
        
        int data;
        for (int i = start; i <= end; i++)
        {
            enqueue(q, i);
            if (!dequeue(q, &data)) seen[data].data++;
        }

        #pragma omp barrier
        #pragma omp master
        {
            gettimeofday(&end_time, NULL);
        }

        #if defined(LOCKFREE) || defined(CK)
        fl_free();
        #endif  
    }

    calc_benchmark(&start_time, &end_time, N_TEST3);

    for (int i = 1; i <= N_TEST3; i++)
    {
        if (seen[i].data != 1)
        {
            printf("data %d was found %d times in queue\n", i, seen[i].data);
            exit(1);
        }
    }

    int *seen2 = calloc(N_TEST3 * 2 + 1, sizeof(int));
    int data;
    for (int i = 0; i < N_TEST3; i++)
    {
        if (dequeue(q, &data)) break;
        if (data < N_TEST3 || data > N_TEST3 * 2)
        {
            printf("invalid data %d in queue\n", data);
            exit(1);
        }
        seen2[data]++;
    }

    if (!dequeue(q, &data))
    {
        printf("queue has more than %d items\n", N_TEST3);
        exit(1);
    }

    for (int i = N_TEST3 + 1; i <= N_TEST3 * 2; i++)
    {
        if (seen2[i] != 1)
        {
            printf("data %d was found %d times in queue\n", i, seen2[i]);
            exit(1);
        }
    }

    free(seen);
    free(seen2);
    #if defined(LOCKFREE) || defined(CK)
    fl_free();
    #endif
    queue_free(q);
}

int main(int argc, const char *argv[])
{
    if (argc != 2) 
    {
        printf("Usage: %s <num of threads>\n", argv[0]);
        exit(1);
    }

    nthr = atoi(argv[1]);

    // N_TEST1 CONCURRENT ENQUEUES

    test1();

    // N_TEST2 CONCURRENT DEQUEUES

    test2();

    // N_TEST3 CONCURRENT ENQUEUES THEN ENQUEUES/DEQUEUES

    test3();

    return 0;
}
