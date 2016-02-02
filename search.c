#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>

#include "que.h"

#define MAX_THREADS 1024

static int N_PRODUCERS = 0;
static int N_CONSUMERS = 0;
int producers_working = 0;
static int debug = 0;

#define NUM_CONSUMER_THREADS 2

typedef struct Thread_data_struct
{
    int id;
    pthread_t thread_id;
    char pattern[BUFSIZ];
    char file[BUFSIZ];
} Thread_data;

Thread_data producer_thread_args[MAX_THREADS];
Thread_data consumer_thread_args[MAX_THREADS];

void error(char *msg, char *arg)
{
    printf("**** ERROR: %s '%s'\n", msg, arg);
    pthread_exit(0);
}

void enter_for_find_occurances(char *pattern, char *string)
{
    Buffer b;
    strcpy(b.string, string);
    que_enq(b);   
    if (debug)
        printf("Searching for '%s' in '%s'\n", pattern, string);
}

void *producer_thread_work(void *arg)
{
    Thread_data * data = (Thread_data *)arg;
    char string[BUFSIZ];
    FILE *fd = fopen(data->file, "r");
    if ( !fd ) error("unable to open file", data->file);
    while (fgets(string, BUFSIZ, fd) )
    {
        int len = strlen(string) - 1;
        if (string[len] == '\n')
            string[len] = '\0';
        enter_for_find_occurances(data->pattern, string);
    }
    fclose(fd);
    pthread_exit(0);
}

void start_producer_thread(char *pattern, char *file, int i)
{
    Thread_data * t = &producer_thread_args[N_PRODUCERS];
    if (debug)
        printf("Creating a producer %d\n", i);
    t->id = N_PRODUCERS++;
    strcpy(t->pattern, pattern);
    strcpy(t->file, file);
    pthread_create(&t->thread_id, NULL, producer_thread_work, t);
}

void join_producer_threads(Thread_data producer_thread_args[])
{
    for ( int i=0; i < N_PRODUCERS; i++ )
    {
        if (debug)
            printf("Joining a producer %d\n", i);
        pthread_join(producer_thread_args[i].thread_id, NULL);
    }
}


int match(char *pat, char *str)
{
    return strstr(str, pat) != NULL;
}


void *consumer_thread_work(void *arg)
{
    Thread_data * data = (Thread_data *)arg;
    while (producers_working || !que_is_empty())
    {
        Buffer b = que_deq();
        if (match(data->pattern, b.string))
        {
            if (debug)
                printf("Matched '%s' in '%s'\n", data->pattern, b.string);
            add_match();
        }
    }

    pthread_exit(0);
}

void start_consumer_thread(char *pattern, int i)
{
    Thread_data * t = &consumer_thread_args[N_CONSUMERS];
    if (debug)
        printf("Creating a consumer %d\n", i);
    t->id = N_CONSUMERS++;
    strcpy(t->pattern, pattern);
    pthread_create(&t->thread_id, NULL, consumer_thread_work, t);
}

void join_consumer_threads(Thread_data consumer_thread_args[])
{
    for ( int i=0; i < N_CONSUMERS; i++ )
    {
        if (debug)
            printf("joining a consumer %d\n", i);
        pthread_join(consumer_thread_args[i].thread_id, NULL);
    }
}

int main(int argc, char *argv[])
{
    que_init();
    if ( argc < 3 )
    {
        fprintf(stderr, "Usage: %s string file [file ...]\n", argv[0]);
        exit(-1);
    }

    char *pattern = argv[1];
    producers_working = 1;
    for (int i=2, j=0; i<argc; ++i)
        start_producer_thread(pattern, argv[i], j);
    for (int i=0; i<NUM_CONSUMER_THREADS; ++i)
        start_consumer_thread(pattern, i);
    join_producer_threads(producer_thread_args);
    producers_working = 0;
    join_consumer_threads(consumer_thread_args);
    report_matches(pattern);
    pthread_exit(0);
}
