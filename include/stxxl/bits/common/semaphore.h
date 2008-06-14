/***************************************************************************
 *            semaphore.h
 *
 *  Sat Aug 24 23:53:49 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#ifndef STXXL_SEMAPHORE_HEADER
#define STXXL_SEMAPHORE_HEADER

#ifdef STXXL_BOOST_THREADS
 #include <boost/thread/mutex.hpp>
 #include <boost/thread/condition.hpp>
#else
 #include <pthread.h>
#endif

#include <stxxl/bits/noncopyable.h>
#include "stxxl/bits/common/utils.h"


__STXXL_BEGIN_NAMESPACE

class semaphore : private noncopyable
{
    int v;
#ifdef STXXL_BOOST_THREADS
    boost::mutex mutex;
    boost::condition cond;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif

public:
    semaphore (int init_value = 1) : v (init_value)
    {
#ifndef STXXL_BOOST_THREADS
        check_pthread_call(pthread_mutex_init(&mutex, NULL));
        check_pthread_call(pthread_cond_init(&cond, NULL));
#endif
    }
    ~semaphore ()
    {
#ifndef STXXL_BOOST_THREADS
        int res = pthread_mutex_trylock (&mutex);

        if (res == 0 || res == EBUSY) {
            check_pthread_call(pthread_mutex_unlock(&mutex));
        } else
            stxxl_function_error(resource_error);
        check_pthread_call(pthread_mutex_destroy(&mutex));
        check_pthread_call(pthread_cond_destroy(&cond));
#endif
    }
    // function increments the semaphore and signals any threads that
    // are blocked waiting a change in the semaphore
    int operator ++ (int)
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        int res = ++v;
        Lock.unlock();
        cond.notify_one();
#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        int res = ++v;
        check_pthread_call(pthread_mutex_unlock(&mutex));
        check_pthread_call(pthread_cond_signal(&cond));
#endif
        return res;
    }
    // function decrements the semaphore and blocks if the semaphore is
    // <= 0 until another thread signals a change
    int operator -- (int)
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        while (v <= 0)
            cond.wait(Lock);

        int res = --v;
#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        while (v <= 0)
            check_pthread_call(pthread_cond_wait(&cond, &mutex));

        int res = --v;
        check_pthread_call(pthread_mutex_unlock(&mutex));
#endif
        return res;
    }
    // function does NOT block but simply decrements the semaphore
    // should not be used instead of down -- only for programs where
    // multiple threads must up on a semaphore before another thread
    // can go down, i.e., allows programmer to set the semaphore to
    // a negative value prior to using it for synchronization.
    int decrement ()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        return (--v);
#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        int res = --v;
        check_pthread_call(pthread_mutex_unlock(&mutex));
        return res;
#endif
    }
    // function returns the value of the semaphore at the time the
    // critical section is accessed.  obviously the value is not guaranteed
    // after the function unlocks the critical section.
    //int operator()
    //{
    //      check_pthread_call(pthread_mutex_lock(&mutex));
    //      int res = v;
    //      check_pthread_call(pthread_mutex_unlock(&mutex));
    //      return res;
    //};
};

__STXXL_END_NAMESPACE

#endif // !STXXL_SEMAPHORE_HEADER
