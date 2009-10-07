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

aio_queue::aio_queue(int max_sim_requests) : max_sim_requests(max_sim_requests), posted_free_sem(max_sim_requests), posted_sem(0)
{
    start_thread(post_async, static_cast<void*>(this));
    check_pthread_call(pthread_create(&wait_thread, NULL, wait_async, static_cast<void*>(this)));
}

aio_queue::~aio_queue()
{
    stop_thread();
    check_pthread_call(pthread_join(wait_thread, NULL));
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

void aio_queue::post_requests()
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
				posted_free_sem--;

			while (!static_cast<aio_request*>(req.get())->post())
				;	//FIXME: busy waiting

			{
				scoped_mutex_lock lock(posted_mtx);
				posted_requests.push_back(req);
				posted_sem++;
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

void aio_queue::wait_requests()
{
    request_ptr req;

    for ( ; ; )
    {
        posted_sem--;

		scoped_mutex_lock lock(posted_mtx);
		//grab requests
		aiocb64** prs;
		int pc;
		prs = new aiocb64*[posted_requests.size()];

		pc = 0;
		for (queue_type::const_iterator pr = posted_requests.begin(); pr != posted_requests.end(); ++pr)
			prs[pc++] = (static_cast<aio_request*>((*pr).get()))->get_cb();

		lock.unlock();

		//wait for at least one of them to finish
		aio_suspend64(prs, pc, NULL);
		delete[] prs;

		lock.lock();

		posted_sem++;	//compensate for the one eaten too early above

		//complete finished requests
		for (queue_type::iterator pr = posted_requests.begin(); pr != posted_requests.end(); )
			if (aio_error64((static_cast<aio_request*>((*pr).get()))->get_cb()) != EINPROGRESS)
			{
				(*pr)->completed();
				queue_type::iterator current = pr;
				++pr;
				posted_requests.erase(current);
				if (max_sim_requests != 0)
					posted_free_sem++;
				posted_sem--;
			}

		lock.unlock();
    }
}

void* aio_queue::post_async(void* arg)
{
    (static_cast<aio_queue*>(arg))->post_requests();
    return NULL;
}

void* aio_queue::wait_async(void* arg)
{
    (static_cast<aio_queue*>(arg))->wait_requests();
    return NULL;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
