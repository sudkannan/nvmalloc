#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>
#include <assert.h>
#include "error.h"

int (*__lib_pthread_create)(pthread_t *thread, const pthread_attr_t *attr,
                              void *(*start_routine) (void *), void *arg);
int (*__lib_pthread_mutex_lock)(pthread_mutex_t *mutex);
int (*__lib_pthread_mutex_trylock)(pthread_mutex_t *mutex);
int (*__lib_pthread_mutex_unlock)(pthread_mutex_t *mutex);

int init_interposition()
{
    // if no symbol is returned then no interposition needed
    __lib_pthread_create = dlsym(RTLD_NEXT, "pthread_create");
    __lib_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    __lib_pthread_mutex_trylock = dlsym(RTLD_NEXT, "pthread_mutex_trylock");
    __lib_pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");

    return E_SUCCESS;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg)
{
    int ret;
    DBG_LOG(DEBUG, "interposing pthread_create\n");

    assert(__lib_pthread_create);
    if (ret = __lib_pthread_create(thread, attr, start_routine, arg) != 0) {

    }
    
    register_thread(thread);

    return ret;    
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    DBG_LOG(DEBUG, "interposing pthread_mutex_lock\n");

    assert(__lib_pthread_mutex_lock);
    return __lib_pthread_mutex_lock(mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    DBG_LOG(DEBUG, "interposing pthread_mutex_trylock\n");

    assert(__lib_pthread_mutex_trylock);
    return __lib_pthread_mutex_trylock(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    DBG_LOG(DEBUG, "interposing pthread_mutex_unlock\n");
    
    assert(__lib_pthread_mutex_unlock);
    return __lib_pthread_mutex_unlock(mutex);
}
