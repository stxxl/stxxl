/***************************************************************************
 *  include/stxxl/bits/io/request_queue_impl_qwqr.h
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

#ifndef STXXL_IO_AIO_QUEUE_HEADER
#define STXXL_IO_AIO_QUEUE_HEADER

#include <list>

#include <stxxl/bits/io/request_queue_impl_worker.h>
#include <stxxl/bits/common/mutex.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

class aio_queue : public request_queue_impl_worker, public disk_queue, public singleton<aio_queue>
{
private:
    typedef std::list<request_ptr> queue_type;

    mutex mtx;
    queue_type waiting_requests, posted_requests;
    int max_sim_requests, num_sim_requests;

    static const priority_op _priority_op = WRITE;

    static void* worker(void * arg);
    void handle_requests();
    void suspend();

public:
    // \param max_sim_requests max number of requests simultaneously submitted to disk, 0 means as many as possible
    aio_queue(int max_sim_requests = 0);

    void add_request(request_ptr & req);
    bool cancel_request(request_ptr & req);
    void complete_request(request_ptr & req);
    ~aio_queue();
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_AIO_QUEUE_HEADER
// vim: et:ts=4:sw=4
