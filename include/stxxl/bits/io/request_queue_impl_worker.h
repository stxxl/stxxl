/***************************************************************************
 *  include/stxxl/bits/io/request_queue_impl_worker.h
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

#ifndef STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER
#define STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
 #include <boost/thread/thread.hpp>
#else
 #include <pthread.h>
#endif

#include <stxxl/bits/io/request_queue.h>
#include <stxxl/bits/common/semaphore.h>
#include <stxxl/bits/common/state.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

class request_queue_impl_worker : public request_queue
{
protected:
    enum thread_state { NOT_RUNNING, RUNNING, TERMINATING, TERMINATE = TERMINATING };

    state<thread_state> _thread_state;
    semaphore sem;

#ifdef STXXL_BOOST_THREADS
    boost::thread * thread;
#else
    pthread_t thread;
#endif

protected:
    request_queue_impl_worker() : _thread_state(NOT_RUNNING), sem(0) { }
    ~request_queue_impl_worker() { }
    void start_thread(void * (*worker)(void *), void * arg);
    void stop_thread();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER
// vim: et:ts=4:sw=4
