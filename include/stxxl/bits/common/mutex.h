/***************************************************************************
 *  include/stxxl/bits/common/mutex.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_MUTEX_HEADER
#define STXXL_MUTEX_HEADER

#include <stxxl/bits/config.h>
#include <stxxl/bits/namespace.h>

#if STXXL_STD_THREADS
 #include <mutex>
#elif STXXL_BOOST_THREADS
 #include <boost/thread/mutex.hpp>
#elif STXXL_POSIX_THREADS
 #include <pthread.h>
 #include <cerrno>

 #include <stxxl/bits/noncopyable.h>
 #include <stxxl/bits/common/error_handling.h>
#else
 #error "Thread implementation not detected."
#endif


__STXXL_BEGIN_NAMESPACE

#if STXXL_STD_THREADS

typedef std::mutex mutex;

#elif STXXL_BOOST_THREADS

typedef boost::mutex mutex;

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

#if STXXL_STD_THREADS

typedef std::unique_lock<std::mutex> scoped_mutex_lock;

#elif STXXL_BOOST_THREADS

typedef boost::mutex::scoped_lock scoped_mutex_lock;

#else

//! Aquire a lock that's valid until the end of scope.
class scoped_mutex_lock
{
    mutex & mtx;
    bool is_locked;

public:
    scoped_mutex_lock(mutex & mtx_)
        : mtx(mtx_), is_locked(true)
    {
        mtx.lock();
    }

    ~scoped_mutex_lock()
    {
        unlock();
    }

    void unlock()
    {
        if (is_locked) {
            is_locked = false;
            mtx.unlock();
        }
    }
};

#endif

__STXXL_END_NAMESPACE

#endif // !STXXL_MUTEX_HEADER
