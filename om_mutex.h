#ifndef OM_MUTEX_H_
#define OM_MUTEX_H_

/* Implements locking mechanism on lock allocated by user.
 *  Can be used for
 *   1. process shared lock using either atomic variables or using posix
 *   thread.
 *   The memory used to initalize atomic flag or pthread mutex should
 *   be from
 *   shared memory between two or more processes.
 *   2. simple interface to create and use locking mechanism.
 */

/* comment below line to default to pthread mutex, as c11 compatible compilers
 * only support atomic. */
#define USE_ATOMIC

/* sizeof(pthread_mutex_t) is 40 on my Linux box. To round off, I am making 127. */
#define  MIN_LOCK_SIZE  127

#define  PROCESS_SHARED_LOCK volatile void*
PROCESS_SHARED_LOCK Do_Lock_Init(volatile void *ptr, int sz);
int Do_Lock(PROCESS_SHARED_LOCK k);
int Do_UnLock(PROCESS_SHARED_LOCK k);
int Do_Lock_Destroy(PROCESS_SHARED_LOCK k);

#endif //OM_MUTEX_H_
