/***************************************************************************
 *  include/stxxl/bits/common/condition_variable.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_CONDITION_VARIABLE_HEADER
#define STXXL_COMMON_CONDITION_VARIABLE_HEADER

#include <stxxl/bits/config.h>
#include <stxxl/bits/common/mutex.h>

#if STXXL_STD_THREADS
 #include <condition_variable>
#elif STXXL_BOOST_THREADS
 #include <boost/thread/condition.hpp>
#elif STXXL_POSIX_THREADS
 #include <pthread.h>
 #include <cerrno>

 #include <stxxl/bits/common/error_handling.h>
#else
 #error "Thread implementation not detected."
#endif

namespace stxxl {

#if STXXL_STD_THREADS

typedef std::condition_variable condition_variable;

#elif STXXL_BOOST_THREADS

typedef boost::condition condition_variable;

#elif STXXL_POSIX_THREADS

class condition_variable
{
    //! pthread handle to condition
    pthread_cond_t cond;

public:
    //! initialize condition variable
    condition_variable() throw()
    {
        STXXL_CHECK_PTHREAD_CALL(pthread_cond_init(&cond, NULL));
    }
    //! non-copyable: delete copy-constructor
    condition_variable(const condition_variable &) = delete;
    //! non-copyable: delete assignment operator
    condition_variable & operator = (const condition_variable &) = delete;
    //! destroy condition variable
    ~condition_variable() throw()
    {
        STXXL_CHECK_PTHREAD_CALL(pthread_cond_destroy(&cond));
    }
    //! notify one waiting thread
    void notify_one() throw()
    {
        STXXL_CHECK_PTHREAD_CALL(pthread_cond_signal(&cond));
    }
    //! notify all waiting threads
    void notify_all() throw()
    {
        STXXL_CHECK_PTHREAD_CALL(pthread_cond_broadcast(&cond));
    }
    //! wait for a signal on the condition variable
    void wait(scoped_mutex_lock& lock) throw()
    {
        STXXL_CHECK_PTHREAD_CALL(pthread_cond_wait(&cond, &lock.native_handle()));
    }
};

#endif

} // namespace stxxl

#endif // !STXXL_COMMON_CONDITION_VARIABLE_HEADER
