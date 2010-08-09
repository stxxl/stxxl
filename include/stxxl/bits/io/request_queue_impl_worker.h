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

#ifdef STXXL_BOOST_THREADS
    typedef boost::thread* thread_type;
#else
    typedef pthread_t thread_type;
#endif

protected:
    void start_thread(void * (*worker)(void *), void * arg, thread_type & t, state<thread_state> & s);
    void stop_thread(thread_type & t, state<thread_state> & s, semaphore & sem);
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER
// vim: et:ts=4:sw=4
