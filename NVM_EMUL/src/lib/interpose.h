#ifndef __INTERPOSE_H
#define __INTERPOSE_H

extern int (*__lib_pthread_create)(pthread_t *thread, const pthread_attr_t *attr,
                                   void *(*start_routine) (void *), void *arg);
extern int (*__lib_pthread_mutex_lock)(pthread_mutex_t *mutex);
extern int (*__lib_pthread_mutex_trylock)(pthread_mutex_t *mutex);
extern int (*__lib_pthread_mutex_unlock)(pthread_mutex_t *mutex);

int init_interposition();

#endif /* __INTERPOSE_H */
