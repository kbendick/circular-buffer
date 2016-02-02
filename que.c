#define _POSIX_C_SOURCE 200112L

/*
 * circular-buffer
 * Author: Kyle Bendickson.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include "que.h"


static ELE _que[QUE_MAX];
static int _front = 0, _rear = 0;
extern int producers_working;

static int matches = 0;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER, timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t count_sem, space_sem;

struct timespec timer;


static int debug = 0;

void add_match()
{
    //Note: you will need to lock this update because it is a race condition
    pthread_mutex_lock(&lock);
    if (debug)
        printf("Incrementing the matches from %d\n", matches);
    matches++;
    pthread_mutex_unlock(&lock);
}

void report_matches(char *pattern)
{
    printf("Found %d total matches of '%s'\n", matches, pattern);
}

int que_init()
{
    sem_init(&count_sem, 0, 0);
    sem_init(&space_sem, 0, QUE_MAX-1);
}

void que_error(char *msg)
{
    fprintf(stderr, "***** Error: %s\n", msg);
    //exit(-1);
}

int que_is_full()
{
    return (_rear + 1) % QUE_MAX == _front; /* this is why one slot is unused */
}

int que_is_empty()
{
    return _front == _rear;
}

void que_enq(ELE v)
{

    // wait for space to be available
    sem_wait(&space_sem);

    // lock to prevent a race condition
    pthread_mutex_lock(&lock);

    _que[_rear++] = v;
    if ( _rear >= QUE_MAX )
        _rear = 0;

    pthread_mutex_unlock(&lock);

    // Increment the count of full buffer.
    sem_post(&count_sem);
}

ELE que_deq()
{

    pthread_mutex_lock(&timer_mutex);
    timer.tv_sec = time(NULL);
    timer.tv_sec += 1;
    pthread_mutex_unlock(&timer_mutex);
   
    if( sem_timedwait(&count_sem, &timer) != 0) {
        ELE empty_buffer[] = {'\0'};
        return *empty_buffer;
    }

    if ( que_is_empty() )
        que_error("deq on empty queue");

    // Lock to prevent a race condition
    pthread_mutex_lock(&lock);
    ELE ret = _que[_front++];
    if ( _front >= QUE_MAX )
        _front = 0;
    pthread_mutex_unlock(&lock);

    sem_post(&space_sem);



    return ret;
}


/*

int main()
{
    for ( int i=0; i<QUE_MAX-1; ++i )
    {
        Buffer b;
        sprintf(&b.string, "%d", i);
        que_enq(b);
    }
    while ( !que_is_empty() )
        printf("%s ", que_deq().string);
    putchar('\n');
}
*/
