/***************************************************************************
 *  include/stxxl/bits/io/request_queue_impl_worker.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER
#define STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER

#include <stxxl/bits/common/semaphore.h>
#include <stxxl/bits/common/shared_state.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/io/request_queue.h>

#include <thread>

namespace stxxl {

//! \addtogroup reqlayer
//! \{

//! Implementation of request queue worker threads. Worker threads can be
//! started by start_thread and stopped with stop_thread. The queue state is
//! checked before termination and updated afterwards.
class request_queue_impl_worker : public request_queue
{
protected:
    enum thread_state { NOT_RUNNING, RUNNING, TERMINATING, TERMINATED };

protected:
    void start_thread(
        void* (*worker)(void*), void* arg,
        std::thread& t, shared_state<thread_state>& s);

    void stop_thread(
        std::thread& t, shared_state<thread_state>& s, semaphore& sem);
};

//! \}

} // namespace stxxl

#endif // !STXXL_IO_REQUEST_QUEUE_IMPL_WORKER_HEADER
// vim: et:ts=4:sw=4
