/***************************************************************************
 *  io/request_queue_impl_1q.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <algorithm>
#include <stxxl/bits/io/request_state_impl_basic.h>
#include <stxxl/bits/io/request_queue_impl_1q.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/parallel.h>


#ifndef STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
#define STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 1
#endif

__STXXL_BEGIN_NAMESPACE

struct file_offset_match : public std::binary_function<request_ptr, request_ptr, bool>
{
    bool operator () (
        const request_ptr & a,
        const request_ptr & b) const
    {
        // matching file and offset are enough to cause problems
        return (a->get_offset() == b->get_offset()) &&
               (a->get_file() == b->get_file());
    }
};

request_queue_impl_1q::request_queue_impl_1q(int /*n*/)              //  n is ignored
{
    start_thread(worker, static_cast<void *>(this));
}

void request_queue_impl_1q::add_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request submitted to not running queue.");

#if STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
        {
            scoped_mutex_lock Lock(queue_mutex);
            if (std::find_if(queue.begin(), queue.end(),
                             bind2nd(file_offset_match(), req) __STXXL_FORCE_SEQUENTIAL)
                != queue.end())
            {
                STXXL_ERRMSG("request submitted for a BID with a pending request");
            }
        }
#endif
    scoped_mutex_lock Lock(queue_mutex);
    queue.push_back(req);

    sem++;
}

bool request_queue_impl_1q::cancel_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request cancelled disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request cancelled to not running queue.");

    bool was_still_in_queue = false;
    {
        scoped_mutex_lock Lock(queue_mutex);
        queue_type::iterator pos;
        if ((pos = std::find(queue.begin(), queue.end(), req __STXXL_FORCE_SEQUENTIAL)) != queue.end())
        {
            queue.erase(pos);
            was_still_in_queue = true;
            sem--;
        }
    }

    return was_still_in_queue;
}

request_queue_impl_1q::~request_queue_impl_1q()
{
    stop_thread();
}

void * request_queue_impl_1q::worker(void * arg)
{
    self * pthis = static_cast<self *>(arg);
    request_ptr req;

    for ( ; ; )
    {
        pthis->sem--;

        {
            scoped_mutex_lock Lock(pthis->queue_mutex);
            if (!pthis->queue.empty())
            {
                req = pthis->queue.front();
                pthis->queue.pop_front();

                Lock.unlock();

                //assert(req->nref() > 1);
                req->serve();
            }
            else
            {
                Lock.unlock();

                pthis->sem++;
            }
        }

        // terminate if it has been requested and queues are empty
        if (pthis->_thread_state() == TERMINATE) {
            if ((pthis->sem--) == 0)
                break;
            else
                pthis->sem++;
        }
    }

    return NULL;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
