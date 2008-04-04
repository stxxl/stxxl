#ifndef MUTEX_HEADER
#define MUTEX_HEADER

/***************************************************************************
 *            mutex.h
 *
 *  Sat Aug 24 23:53:20 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#ifdef STXXL_BOOST_THREADS

// stxxl::mutex is not needed since boost:mutex exists
 #include <boost/thread/mutex.hpp>

#else

 #include <pthread.h>

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/utils.h>

#endif


__STXXL_BEGIN_NAMESPACE

#ifdef STXXL_BOOST_THREADS

#else

class mutex : private noncopyable
{
    pthread_mutex_t _mutex;
public:

    mutex ()
    {
        check_pthread_call(pthread_mutex_init(&_mutex, NULL));
    };

    ~mutex ()
    {
        int res = pthread_mutex_trylock (&_mutex);

        if (res == 0 || res == EBUSY) {
            check_pthread_call(pthread_mutex_unlock(&_mutex));
        } else
            stxxl_function_error(resource_error);

        check_pthread_call(pthread_mutex_destroy(&_mutex));
    };
    void lock ()
    {
        check_pthread_call(pthread_mutex_lock (&_mutex));
    }
    void unlock ()
    {
        check_pthread_call(pthread_mutex_unlock (&_mutex));
    }
};

#endif

__STXXL_END_NAMESPACE

#endif
