/***************************************************************************
 *  io/diskqueue.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/request_queue_impl_worker.h>
#include <stxxl/bits/io/request.h>

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
 #include <boost/bind.hpp>
#endif


__STXXL_BEGIN_NAMESPACE

void request_queue_impl_worker::start_thread(void * (*worker)(void *), void * arg)
#ifdef STXXL_BOOST_THREADS
#endif
{
    assert(_thread_state() == NOT_RUNNING);
#ifdef STXXL_BOOST_THREADS
    thread = new boost::thread(boost::bind(worker, arg));
#else
    check_pthread_call(pthread_create(&thread, NULL, worker, arg));
#endif
    _thread_state.set_to(RUNNING);
}

void request_queue_impl_worker::stop_thread()
{
    assert(_thread_state() == RUNNING);
    _thread_state.set_to(TERMINATING);
    sem++;
#ifdef STXXL_BOOST_THREADS
    thread->join();
    delete thread;
    thread = NULL;
#else
    check_pthread_call(pthread_join(thread, NULL));
#endif
    _thread_state.set_to(NOT_RUNNING);
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
