#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>

#include "om_mutex.h"
/* Implements
 * process shared lock using either atomic variables or using posix thread.
 * The memory used to initalize atomic flag or pthread mutex should be from
 * shared memory between two or more processes.
 */

/*
 * sources:
 * http://stackoverflow.com/questions/20209021/what-is-the-difference-between-using-atomic-flag-init-and-stdatomic-flagclea
 * https://gustedt.wordpress.com/2012/03/21/c11-defects-initialization-of-atomic_flag/
 *  http://en.cppreference.com/w/c/atomic/atomic_flag_test_and_set
 *  https://gustedt.wordpress.com/2012/01/22/simple-c11-atomics-atomic_flag/
 *  http://nullprogram.com/blog/2014/09/02/
 */

#ifndef USE_ATOMIC
static int singleton = 0;
static pthread_mutexattr_t mutex_shared_attr;

#endif

typedef struct Lock_s {
#ifdef USE_ATOMIC
atomic_flag cat;
#else
pthread_mutex cm;
#endif
int max_bound;
}Lock_s;

PROCESS_SHARED_LOCK Do_Lock_Init(volatile void *ptr, int sz)
{
//    printf("sz is %d while MIN_SIZE is %d", sz, MIN_LOCK_SIZE);
    assert(sz >=  MIN_LOCK_SIZE);
#ifdef USE_ATOMIC
    memset((void*)ptr, 0, sz);
    volatile Lock_s *s = ptr;
    atomic_flag *cat = (atomic_flag *)&(s->cat);
#else

    if (0 == singleton)
    {
        int rtn;

        if (( rtn = pthread_mutexattr_init(&mutex_shared_attr)) != 0)
            assert (0);

        if (( rtn = pthread_mutexattr_setpshared(&mutex_shared_attr,PTHREAD_PROCESS_SHARED)) != 0)
            assert(0);

        singleton = 1;
    }

    int rtn;
    Lock_s *s = ptr;
    if (( rtn = pthread_mutex_init(&(s->cm), &mutex_shared_attr)) != 0)
        assert(0);

    pthread_mutex_t *cat  = (pthread_mutex_t *) &(s->cm);
#endif
    return (void *) s;
}

int Do_Lock(PROCESS_SHARED_LOCK k)
{
  volatile Lock_s *s = (Lock_s*)k;

#ifdef USE_ATOMIC
    do {}
    while (atomic_flag_test_and_set((volatile atomic_flag *)&(s->cat)));

#else
    int rtn;

    if ((rtn = pthread_mutex_lock((pthread_mutex_t *)&(s->cm))) != 0)
    {
        return 1;
    }

#endif
    return 0;
}

int Do_UnLock(PROCESS_SHARED_LOCK k)
{
  volatile Lock_s *s = (Lock_s*)k;
#ifdef USE_ATOMIC
    atomic_flag_clear((volatile atomic_flag *)&(s->cat));
#else
    int rtn;

    if ((rtn = pthread_mutex_unlock((pthread_mutex_t *)&(s->cm))) != 0)
    {
        return 1;
    }

#endif
    return 0;
}

int Do_Lock_Destroy(PROCESS_SHARED_LOCK s)
{
#ifdef USE_ATOMIC
    /* No-op. */
#else
    /* No-op. */
#endif
    return 0;
}

#if 0
int main(void)
{
    void *t = calloc(1, MIN_LOCK_SIZE);
    PROCESS_SHARED_LOCK k = Do_Lock_Init(t, MIN_LOCK_SIZE);
    assert(NULL != k);
    printf("done init\n");
    assert(0 == Do_Lock(k));
    printf("done lock\n");
    assert(0 == Do_UnLock(k));
    printf( "hi\n");
    Do_Lock_Destroy(k);
    return 0;
}

#endif
