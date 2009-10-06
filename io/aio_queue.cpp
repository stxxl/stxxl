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

aio_queue::aio_queue(int max_sim_requests) : max_sim_requests(max_sim_requests), num_sim_requests(0)
{
    start_thread(worker, static_cast<void *>(this));
}

void aio_queue::add_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request submitted to not running queue.");

	scoped_mutex_lock lock(mtx);
   	waiting_requests.push_back(req);

    sem++;
}

bool aio_queue::cancel_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request canceled to not running queue.");

    bool was_still_in_queue = false;
	scoped_mutex_lock lock(mtx);
	queue_type::iterator pos;
	if ((pos = std::find(waiting_requests.begin(), waiting_requests.end(), req __STXXL_FORCE_SEQUENTIAL)) != waiting_requests.end())
	{
		waiting_requests.erase(pos);
		was_still_in_queue = true;
		sem--;
	}

    return was_still_in_queue;
}

aio_queue::~aio_queue()
{
    stop_thread();
}

void aio_queue::suspend()
{
	aiocb64** prs = new aiocb64*[posted_requests.size()];

	int i = 0;
	for (queue_type::const_iterator pr = posted_requests.begin(); pr != posted_requests.end(); ++pr)
		prs[i++] = (static_cast<aio_request*>((*pr).get()))->get_cb();

	aio_suspend(NULL, 0, NULL);
	aio_suspend64(prs, posted_requests.size(), NULL);
}

void aio_queue::handle_requests()
{
    aio_request* req;

    for ( ; ; )
    {
        sem--;

		scoped_mutex_lock lock(mtx);
		if (!waiting_requests.empty())
		{
			req = static_cast<aio_request*>(waiting_requests.front().get());

			lock.unlock();

			if(!req->post())
				suspend();
			else
			{
				waiting_requests.pop_front();
				posted_requests.push_back(req);
			}
		}
		else
		{
			lock.unlock();

			sem++;
		}

        // terminate if termination has been requested and queues are empty
        if (_thread_state() == TERMINATE) {
            if ((sem--) == 0)
                break;
            else
                sem++;
        }
    }
}

void* aio_queue::worker(void * arg)
{
    (static_cast<aio_queue*>(arg))->handle_requests();
    return NULL;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
