#ifndef STATE_HEADER
#define STATE_HEADER

/***************************************************************************
 *            state.h
 *
 *  Sat Aug 24 23:53:53 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include <stxxl/bits/noncopyable.h>
#include "stxxl/bits/common/utils.h"

#ifdef STXXL_BOOST_THREADS
 #include <boost/thread/mutex.hpp>
 #include <boost/thread/condition.hpp>
#else
 #include <pthread.h>
#endif


__STXXL_BEGIN_NAMESPACE

class state : private noncopyable
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex mutex;
    boost::condition cond;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
    int _state;

public:
    state (int s = 0) : _state (s)
    {
#ifndef STXXL_BOOST_THREADS
        check_pthread_call(pthread_mutex_init(&mutex, NULL));
        check_pthread_call(pthread_cond_init(&cond, NULL));
#endif
    }
    ~state ()
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
    void set_to (int new_state)
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        _state = new_state;
        Lock.unlock();
        cond.notify_all();
#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        _state = new_state;
        check_pthread_call(pthread_mutex_unlock(&mutex));
        check_pthread_call(pthread_cond_broadcast(&cond));
#endif
    }
    void wait_for (int needed_state)
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        while (needed_state != _state)
            cond.wait(Lock);

#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        while (needed_state != _state)
            check_pthread_call(pthread_cond_wait(&cond, &mutex));

        check_pthread_call(pthread_mutex_unlock(&mutex));
#endif
    }
    int operator ()  ()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        return _state;
#else
        int res;
        check_pthread_call(pthread_mutex_lock(&mutex));
        res = _state;
        check_pthread_call(pthread_mutex_unlock(&mutex));
        return res;
#endif
    }
};

__STXXL_END_NAMESPACE

#endif
