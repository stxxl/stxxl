/***************************************************************************
 *  include/stxxl/bits/io/disk_queues.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_DISK_QUEUE_HEADER
#define STXXL_IO_DISK_QUEUE_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <queue>

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
 #include <boost/thread/thread.hpp>
#else
 #include <pthread.h>
#endif

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/semaphore.h>
#include <stxxl/bits/common/mutex.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

class request_ptr;

class disk_queue
{
public:
    enum priority_op { READ, WRITE, NONE };
};

class request_queue_impl_qwqr : private noncopyable, public disk_queue
{
    typedef request_queue_impl_qwqr self;

    mutex write_mutex;
    mutex read_mutex;
    std::queue<request_ptr> write_queue;
    std::queue<request_ptr> read_queue;

    semaphore sem;

    static const priority_op _priority_op = WRITE;

#ifdef STXXL_BOOST_THREADS
    boost::thread thread;
#else
    pthread_t thread;
#endif

    static void * worker(void * arg);

public:
    // \param n max number of requests simultaneously submitted to disk
    request_queue_impl_qwqr(int n = 1);

    // in a multi-threaded setup this does not work as intended
    // also there were race conditions possible
    // and actually an old value was never restored once a new one was set ...
    // so just disable it and all it's nice implications
    void set_priority_op(priority_op op)
    {
        //_priority_op = op;
        UNUSED(op);
    }
    void add_request(request_ptr & req);
    ~request_queue_impl_qwqr();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_DISK_QUEUE_HEADER
// vim: et:ts=4:sw=4
