/***************************************************************************
 *  include/stxxl/bits/common/mutex.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MUTEX_HEADER
#define STXXL_MUTEX_HEADER

#include <stxxl/bits/namespace.h>

#ifdef STXXL_BOOST_THREADS

 #include <boost/thread/mutex.hpp>

#else

 #include <pthread.h>

 #include <stxxl/bits/noncopyable.h>
 #include <stxxl/bits/common/utils.h>

#endif


__STXXL_BEGIN_NAMESPACE

#ifdef STXXL_BOOST_THREADS

// stxxl::mutex is not needed since boost:mutex exists

#else

class mutex : private noncopyable
{
    pthread_mutex_t _mutex;

public:
    mutex()
    {
        check_pthread_call(pthread_mutex_init(&_mutex, NULL));
    }

    ~mutex()
    {
        int res = pthread_mutex_trylock(&_mutex);

        if (res == 0 || res == EBUSY) {
            check_pthread_call(pthread_mutex_unlock(&_mutex));
        } else
            stxxl_function_error(resource_error);

        check_pthread_call(pthread_mutex_destroy(&_mutex));
    }
    void lock()
    {
        check_pthread_call(pthread_mutex_lock(&_mutex));
    }
    void unlock()
    {
        check_pthread_call(pthread_mutex_unlock(&_mutex));
    }
};

#endif

#ifdef STXXL_BOOST_THREADS

typedef boost::mutex::scoped_lock scoped_mutex_lock;

#else

//! \brief Aquire a lock that's valid until the end of scope
class scoped_mutex_lock
{
    mutex &mtx;
    bool is_locked;

public:
    scoped_mutex_lock(mutex& mtx_) : mtx(mtx_), is_locked(false)
    {
        lock();
    }

    ~scoped_mutex_lock()
    {
        unlock();
    }

    void lock()
    {
        if (!is_locked) {
            mtx.lock();
            is_locked = true;
        }
    }

    void unlock()
    {
        if (is_locked) {
            mtx.unlock();
            is_locked = false;
        }
    }
};

#endif

__STXXL_END_NAMESPACE

#endif // !STXXL_MUTEX_HEADER
