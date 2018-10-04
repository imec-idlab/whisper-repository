/* ------------------------------------------------------------------------------
 * This software is in the public domain, furnished "as is", without technical
 * support, and with no warranty, express or implied, as to its usefulness for
 * any purpose.
 *
 * pthread_spin_lock_shim.h
 * A spinlock implementation
 *
 * Author: Majek (https://twitter.com/majek04)
 *   https://idea.popcount.org/2012-09-12-reinventing-spinlocks/
 *   https://github.com/majek/dump/blob/master/msqueue/pthread_spin_lock_shim.h
 * ----------------------------------------------------------------------------*/

#ifndef PTHREAD_SPIN_LOCK_SHIM
#define PTHREAD_SPIN_LOCK_SHIM

#include <errno.h>

typedef int pthread_spinlock_t;

#ifndef PTHREAD_PROCESS_SHARED
#define PTHREAD_PROCESS_SHARED 1
#endif
#ifndef PTHREAD_PROCESS_PRIVATE
#define PTHREAD_PROCESS_PRIVATE 2
#endif

static inline int
pthread_spin_init(pthread_spinlock_t * lock, int pshared)
{
    __asm__ __volatile__("":::"memory");

    *lock = 0;
    return 0;
}

static inline int
pthread_spin_destroy(pthread_spinlock_t * lock)
{
    return 0;
}

static inline int
pthread_spin_lock(pthread_spinlock_t * lock)
{
    while(1) {
        int i;

        for(i = 0; i < 10000; i++) {
            if(__sync_bool_compare_and_swap(lock, 0, 1)) {
                return 0;
            }
        }
        sched_yield();
    }
}

static inline int
pthread_spin_trylock(pthread_spinlock_t * lock)
{
    if(__sync_bool_compare_and_swap(lock, 0, 1)) {
        return 0;
    }
    return EBUSY;
}

static inline int
pthread_spin_unlock(pthread_spinlock_t * lock)
{
    __asm__ __volatile__("":::"memory");

    *lock = 0;
    return 0;
}

#endif
