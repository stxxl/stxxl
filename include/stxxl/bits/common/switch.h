/***************************************************************************
 *  include/stxxl/bits/common/switch.h
 *
 *  kind of semaphore
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_SWITCH_HEADER
#define STXXL_COMMON_SWITCH_HEADER

#include <stxxl/bits/config.h>

#if STXXL_STD_THREADS
 #include <mutex>
 #include <condition_variable>
#elif STXXL_BOOST_THREADS
 #include <boost/thread/mutex.hpp>
 #include <boost/thread/condition.hpp>
#elif STXXL_POSIX_THREADS
 #include <pthread.h>
#else
 #error "Thread implementation not detected."
#endif

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/error_handling.h>


__STXXL_BEGIN_NAMESPACE

//#define onoff_switch Switch

class onoff_switch : private noncopyable
{
#if STXXL_STD_THREADS
    std::mutex mutex;
    std::condition_variable cond;
    typedef std::unique_lock<std::mutex> scoped_lock;
#elif STXXL_BOOST_THREADS
    boost::mutex mutex;
    boost::condition cond;
    typedef boost::mutex::scoped_lock scoped_lock;
#else
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
    bool _on;

public:
    onoff_switch(bool flag = false) : _on(flag)
    {
#if STXXL_POSIX_THREADS
        check_pthread_call(pthread_mutex_init(&mutex, NULL));
        check_pthread_call(pthread_cond_init(&cond, NULL));
#endif
    }
    ~onoff_switch()
    {
#if STXXL_POSIX_THREADS
        int res = pthread_mutex_trylock(&mutex);

        if (res == 0 || res == EBUSY) {
            check_pthread_call(pthread_mutex_unlock(&mutex));
        } else
            stxxl_function_error(resource_error);
        check_pthread_call(pthread_mutex_destroy(&mutex));


        check_pthread_call(pthread_cond_destroy(&cond));
#endif
    }
    void on()
    {
#if STXXL_STD_THREADS || STXXL_BOOST_THREADS
        scoped_lock Lock(mutex);
        _on = true;
        Lock.unlock();
        cond.notify_one();
#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        _on = true;
        check_pthread_call(pthread_mutex_unlock(&mutex));
        check_pthread_call(pthread_cond_signal(&cond));
#endif
    }
    void off()
    {
#if STXXL_STD_THREADS || STXXL_BOOST_THREADS
        scoped_lock Lock(mutex);
        _on = false;
        Lock.unlock();
        cond.notify_one();
#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        _on = false;
        check_pthread_call(pthread_mutex_unlock(&mutex));
        check_pthread_call(pthread_cond_signal(&cond));
#endif
    }
    void wait_for_on()
    {
#if STXXL_STD_THREADS || STXXL_BOOST_THREADS
        scoped_lock Lock(mutex);
        if (!_on)
            cond.wait(Lock);

#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        if (!_on)
            check_pthread_call(pthread_cond_wait(&cond, &mutex));

        check_pthread_call(pthread_mutex_unlock(&mutex));
#endif
    }
    void wait_for_off()
    {
#if STXXL_STD_THREADS || STXXL_BOOST_THREADS
        scoped_lock Lock(mutex);
        if (_on)
            cond.wait(Lock);

#else
        check_pthread_call(pthread_mutex_lock(&mutex));
        if (_on)
            check_pthread_call(pthread_cond_wait(&cond, &mutex));

        check_pthread_call(pthread_mutex_unlock(&mutex));
#endif
    }
    bool is_on()
    {
#if STXXL_STD_THREADS || STXXL_BOOST_THREADS
        scoped_lock Lock(mutex);
        return _on;
#else
        bool res;
        check_pthread_call(pthread_mutex_lock(&mutex));
        res = _on;
        check_pthread_call(pthread_mutex_unlock(&mutex));
        return res;
#endif
    }
};

__STXXL_END_NAMESPACE

#endif // !STXXL_COMMON_SWITCH_HEADER
