#ifndef SWITCH_HEADER
#define SWITCH_HEADER

/***************************************************************************
 *            switch.h
 *	kind of semaphore
 *  Sat Aug 24 23:53:59 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/common/utils.h"

#ifdef STXXL_BOOST_THREADS
 #include <boost/thread/mutex.hpp>
 #include <boost/thread/condition.hpp>
#else
 #include <pthread.h>
#endif


__STXXL_BEGIN_NAMESPACE

//#define onoff_switch Switch

class onoff_switch
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex mutex;
    boost::condition cond;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
    bool _on;
    onoff_switch(const onoff_switch & obj);
    onoff_switch & operator = (const onoff_switch & obj);
public:
    onoff_switch (bool flag = false) : _on (flag)
    {
#ifndef STXXL_BOOST_THREADS
        stxxl_nassert (pthread_mutex_init (&mutex, NULL), resource_error);
        stxxl_nassert (pthread_cond_init (&cond, NULL), resource_error);
#endif
    };
    ~onoff_switch ()
    {
#ifndef STXXL_BOOST_THREADS
        int res = pthread_mutex_trylock (&mutex);

        if (res == 0 || res == EBUSY) {
            stxxl_nassert (pthread_mutex_unlock
                           (&mutex), resource_error);
        } else
            stxxl_function_error(resource_error);
        stxxl_nassert (pthread_mutex_destroy
                       (&mutex), resource_error);


        stxxl_nassert (pthread_cond_destroy (&cond), resource_error);
#endif
    };
    void on ()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        _on = true;
        Lock.unlock();
        cond.notify_one();
#else
        stxxl_nassert (pthread_mutex_lock (&mutex), resource_error);
        _on = true;
        stxxl_nassert (pthread_mutex_unlock (&mutex), resource_error);
        stxxl_nassert (pthread_cond_signal (&cond), resource_error);
#endif
    }
    void off ()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        _on = false;
        Lock.unlock();
        cond.notify_one();
#else
        stxxl_nassert (pthread_mutex_lock (&mutex), resource_error);
        _on = false;
        stxxl_nassert (pthread_mutex_unlock (&mutex), resource_error);
        stxxl_nassert (pthread_cond_signal (&cond), resource_error);
#endif
    }
    void wait_for_on ()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        if (!_on)
            cond.wait(Lock);

#else
        stxxl_nassert (pthread_mutex_lock (&mutex), resource_error);
        if (!_on)
            stxxl_nassert (pthread_cond_wait
                           (&cond, &mutex), resource_error);

        stxxl_nassert (pthread_mutex_unlock (&mutex), resource_error);
#endif
    }
    void wait_for_off ()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        if (_on)
            cond.wait(Lock);

#else
        stxxl_nassert (pthread_mutex_lock (&mutex), resource_error);
        if (_on)
            stxxl_nassert (pthread_cond_wait
                           (&cond, &mutex), resource_error);

        stxxl_nassert (pthread_mutex_unlock (&mutex), resource_error);
#endif
    }
    bool is_on ()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(mutex);
        return _on;
#else
        bool res;
        stxxl_nassert (pthread_mutex_lock (&mutex), resource_error);
        res = _on;
        stxxl_nassert (pthread_mutex_unlock (&mutex), resource_error);
        return res;
#endif
    }
};

__STXXL_END_NAMESPACE

#endif
