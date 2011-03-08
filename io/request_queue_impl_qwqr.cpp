/***************************************************************************
 *  io/request_queue_impl_qwqr.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <algorithm>
#include <stxxl/bits/io/request_state_impl_basic.h>
#include <stxxl/bits/io/request_queue_impl_qwqr.h>
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

request_queue_impl_qwqr::request_queue_impl_qwqr(int n) : _thread_state(NOT_RUNNING), sem(0)
{
    STXXL_UNUSED(n);
    start_thread(worker, static_cast<void *>(this), thread, _thread_state);
}

void request_queue_impl_qwqr::add_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request submitted to not running queue.");

    if (req.get()->get_type() == request::READ)
    {
#if STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
        {
            scoped_mutex_lock Lock(write_mutex);
            if (std::find_if(write_queue.begin(), write_queue.end(),
                             bind2nd(file_offset_match(), req) _STXXL_FORCE_SEQUENTIAL)
                != write_queue.end())
            {
                STXXL_ERRMSG("READ request submitted for a BID with a pending WRITE request");
            }
        }
#endif
        scoped_mutex_lock Lock(read_mutex);
        read_queue.push_back(req);
    }
    else
    {
#if STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
        {
            scoped_mutex_lock Lock(read_mutex);
            if (std::find_if(read_queue.begin(), read_queue.end(),
                             bind2nd(file_offset_match(), req) _STXXL_FORCE_SEQUENTIAL)
                != read_queue.end())
            {
                STXXL_ERRMSG("WRITE request submitted for a BID with a pending READ request");
            }
        }
#endif
        scoped_mutex_lock Lock(write_mutex);
        write_queue.push_back(req);
    }

    sem++;
}

bool request_queue_impl_qwqr::cancel_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request canceled to not running queue.");

    bool was_still_in_queue = false;
    if (req.get()->get_type() == request::READ)
    {
        scoped_mutex_lock Lock(read_mutex);
        queue_type::iterator pos;
        if ((pos = std::find(read_queue.begin(), read_queue.end(), req _STXXL_FORCE_SEQUENTIAL)) != read_queue.end())
        {
            read_queue.erase(pos);
            was_still_in_queue = true;
            sem--;
        }
    }
    else
    {
        scoped_mutex_lock Lock(write_mutex);
        queue_type::iterator pos;
        if ((pos = std::find(write_queue.begin(), write_queue.end(), req _STXXL_FORCE_SEQUENTIAL)) != write_queue.end())
        {
            write_queue.erase(pos);
            was_still_in_queue = true;
            sem--;
        }
    }

    return was_still_in_queue;
}

request_queue_impl_qwqr::~request_queue_impl_qwqr()
{
    stop_thread(thread, _thread_state, sem);
}

void * request_queue_impl_qwqr::worker(void * arg)
{
    self * pthis = static_cast<self *>(arg);
    request_ptr req;

    bool write_phase = true;
    for ( ; ; )
    {
        pthis->sem--;

        if (write_phase)
        {
            scoped_mutex_lock WriteLock(pthis->write_mutex);
            if (!pthis->write_queue.empty())
            {
                req = pthis->write_queue.front();
                pthis->write_queue.pop_front();

                WriteLock.unlock();

                //assert(req->nref() > 1);
                req->serve();
            }
            else
            {
                WriteLock.unlock();

                pthis->sem++;

                if (pthis->_priority_op == WRITE)
                    write_phase = false;
            }

            if (pthis->_priority_op == NONE
                || pthis->_priority_op == READ)
                write_phase = false;
        }
        else
        {
            scoped_mutex_lock ReadLock(pthis->read_mutex);

            if (!pthis->read_queue.empty())
            {
                req = pthis->read_queue.front();
                pthis->read_queue.pop_front();

                ReadLock.unlock();

                STXXL_VERBOSE2("queue: before serve request has " << req->nref() << " references ");
                //assert(req->nref() > 1);
                req->serve();
                STXXL_VERBOSE2("queue: after serve request has " << req->nref() << " references ");
            }
            else
            {
                ReadLock.unlock();

                pthis->sem++;

                if (pthis->_priority_op == READ)
                    write_phase = true;
            }

            if (pthis->_priority_op == NONE
                || pthis->_priority_op == WRITE)
                write_phase = true;
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
