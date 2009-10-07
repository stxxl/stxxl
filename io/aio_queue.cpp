/***************************************************************************
 *  io/aio_queue.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/request_state_impl_basic.h>
#include <stxxl/bits/io/aio_queue.h>
#include <stxxl/bits/io/aio_request.h>
#include <stxxl/bits/parallel.h>


#ifndef STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
#define STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 1
#endif

__STXXL_BEGIN_NAMESPACE

aio_queue::aio_queue(int max_sim_requests) : max_sim_requests(max_sim_requests), posted_sem(max_sim_requests)
{
    start_thread(worker, static_cast<void*>(this));
}

void aio_queue::add_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request submitted to not running queue.");

	scoped_mutex_lock lock(waiting_mtx);
   	waiting_requests.push_back(req);

    sem++;
}

void aio_queue::complete_request(request_ptr& req)
{
	scoped_mutex_lock lock(posted_mtx);
	std::remove(posted_requests.begin(), posted_requests.end(), req __STXXL_FORCE_SEQUENTIAL);
	if (max_sim_requests != 0)
		posted_sem++;
}

bool aio_queue::cancel_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request canceled to not running queue.");

    bool was_still_in_queue = false;
	queue_type::iterator pos;
	{
		scoped_mutex_lock lock(waiting_mtx);
		if ((pos = std::find(waiting_requests.begin(), waiting_requests.end(), req __STXXL_FORCE_SEQUENTIAL)) != waiting_requests.end())
		{
			waiting_requests.erase(pos);
			was_still_in_queue = true;
			sem--;
		}
	}
	if (!was_still_in_queue)
		return req->cancel();
    return was_still_in_queue;
}

aio_queue::~aio_queue()
{
    stop_thread();
}

void aio_queue::handle_requests()
{
    request_ptr req;

    for ( ; ; )
    {
        sem--;

		scoped_mutex_lock lock(waiting_mtx);
		if (!waiting_requests.empty())
		{
			req = waiting_requests.front();
			waiting_requests.pop_front();
			lock.unlock();

			if (max_sim_requests != 0)
				posted_sem--;

			while (!static_cast<aio_request*>(req.get())->post())
				;	//FIXME: busy waiting

			{
				scoped_mutex_lock lock(posted_mtx);
				posted_requests.push_back(req);
			}
		}
		else
		{
			lock.unlock();

			sem++;
		}

        // terminate if termination has been requested
        if (_thread_state() == TERMINATE) {
            if ((sem--) == 0)
                break;
            else
                sem++;
        }
    }
}

void* aio_queue::worker(void* arg)
{
    (static_cast<aio_queue*>(arg))->handle_requests();
    return NULL;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
