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

 #include "stxxl/bits/common/utils.h"

 #include <pthread.h>


__STXXL_BEGIN_NAMESPACE

class mutex
{
    pthread_mutex_t _mutex;
public:

    mutex ()
    {
        stxxl_nassert (pthread_mutex_init (&_mutex, NULL), resource_error);
    };

    ~mutex ()
    {
        int res = pthread_mutex_trylock (&_mutex);

        if (res == 0 || res == EBUSY)
            stxxl_nassert(pthread_mutex_unlock(&_mutex), resource_error)
            else
                stxxl_function_error(resource_error)

                stxxl_nassert(pthread_mutex_destroy(&_mutex), resource_error);

    };
    void lock ()
    {
        stxxl_nassert(pthread_mutex_lock (&_mutex), resource_error);
    };
    void unlock ()
    {
        stxxl_nassert(pthread_mutex_unlock (&_mutex), resource_error);
    };
};

__STXXL_END_NAMESPACE

#endif

#endif
