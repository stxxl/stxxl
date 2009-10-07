/***************************************************************************
 *  include/stxxl/bits/io/aio_queue.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_AIO_QUEUE_HEADER
#define STXXL_IO_AIO_QUEUE_HEADER

#include <list>

#include <stxxl/bits/io/request_queue_impl_worker.h>
#include <stxxl/bits/common/mutex.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

//! \brief Queue for aio_file(s)
class aio_queue : public request_queue_impl_worker, public disk_queue, public singleton<aio_queue>
{
private:
    typedef std::list<request_ptr> queue_type;

    mutex waiting_mtx, posted_mtx;
    queue_type waiting_requests, posted_requests;
    int max_sim_requests;
    semaphore posted_sem;

    static const priority_op _priority_op = WRITE;

    static void* post_async(void* arg);	//thread start callback
    void post_requests();
    void suspend();

public:
    // \param max_sim_requests max number of requests simultaneously submitted to disk, 0 means as many as possible
    aio_queue(int max_sim_requests = 1024);

    void add_request(request_ptr& req);
    bool cancel_request(request_ptr& req);
    void complete_request(request_ptr& req);
    ~aio_queue();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_AIO_QUEUE_HEADER
// vim: et:ts=4:sw=4
