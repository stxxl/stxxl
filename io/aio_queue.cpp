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

#include <stxxl/bits/mng/mng.h>
#include <stxxl/bits/io/aio_queue.h>

#if STXXL_HAVE_AIO_FILE

#include <sys/syscall.h>

#include <stxxl/bits/io/aio_request.h>
#include <stxxl/bits/parallel.h>

#include <algorithm>


#ifndef STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
#define STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 1
#endif

__STXXL_BEGIN_NAMESPACE

aio_queue::aio_queue(int max_sim_requests) :
    max_sim_requests(max_sim_requests),
    sem(0), posted_free_sem(max_sim_requests), posted_sem(0),
    post_thread_state(NOT_RUNNING), wait_thread_state(NOT_RUNNING)
{
    context = 0;

    if (max_sim_requests == 0)
        max_events = std::max(config::get_instance()->disks_number(), 4u) * 64;
    else
        max_events = max_sim_requests;

    int result;
    while ((result = syscall(SYS_io_setup, max_events, &context)) == -1 && errno == EAGAIN && max_events > 1)
        max_events <<= 1;               // try with half as many events
    if (result != 0)
        STXXL_THROW2(io_error, "io_setup() nr_events=" << max_events);

    //STXXL_MSG("Set up an aio queue with " << max_events << " entries.");

    start_thread(post_async, static_cast<void *>(this), post_thread, post_thread_state);
    start_thread(wait_async, static_cast<void *>(this), wait_thread, wait_thread_state);
}

aio_queue::~aio_queue()
{
    stop_thread(post_thread, post_thread_state, sem);
    stop_thread(wait_thread, wait_thread_state, posted_sem);
    syscall(SYS_io_destroy, context);
}

void aio_queue::add_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (post_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request submitted to not running queue.");

    scoped_mutex_lock lock(waiting_mtx);

    waiting_requests.push_back(req);
    sem++;
}

bool aio_queue::cancel_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (post_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request canceled to not running queue.");

    bool was_still_in_queue = false;
    queue_type::iterator pos;
    {
        scoped_mutex_lock lock(waiting_mtx);
        if ((pos = std::find(waiting_requests.begin(), waiting_requests.end(), req _STXXL_FORCE_SEQUENTIAL)) != waiting_requests.end())
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
    io_event * events = new io_event[max_events];

    for ( ; ; )
    {
        int num_waiting = sem--;

        // terminate if termination has been requested
        if (post_thread_state() == TERMINATE && num_waiting == 0)
            break;

        scoped_mutex_lock lock(waiting_mtx);
        if (!waiting_requests.empty())
        {
            req = waiting_requests.front();
            waiting_requests.pop_front();
            lock.unlock();

            if (max_sim_requests != 0)
                posted_free_sem--;

            while (!static_cast<aio_request *>(req.get())->post())
            {
                int num_events = syscall(SYS_io_getevents, context, 1, max_events, events, NULL);
                if (num_events < 0)
                    STXXL_THROW2(io_error, "io_getevents() nr_events=" << max_events);

                handle_events(events, num_events, false);
            }

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
    }

    delete[] events;
}

void aio_queue::handle_events(io_event * events, int num_events, bool canceled)
{
    for (int e = 0; e < num_events; ++e)
    {
        static_cast<aio_request *>(reinterpret_cast<request_ptr *>(events[e].data)->get())->completed(canceled);
        delete reinterpret_cast<request_ptr *>(events[e].data);         // release auto_ptr reference
        if (max_sim_requests != 0)
            posted_free_sem++;
        posted_sem--;
    }
}

void aio_queue::wait_requests()
{
    request_ptr req;
    io_event * events = new io_event[max_events];

    for ( ; ; )
    {
        int num_posted = posted_sem--;

        // terminate if termination has been requested
        if (wait_thread_state() == TERMINATE && num_posted == 0)
            break;

        // wait for at least one of them to finish
        int num_events = syscall(SYS_io_getevents, context, 1, max_events, events, NULL);
        if (num_events < 0)
            STXXL_THROW2(io_error, "io_getevents() nr_events=" << max_events);

        posted_sem++;           // compensate for the one eaten too early above

        handle_events(events, num_events, false);
    }

    delete[] events;
}

void * aio_queue::post_async(void * arg)
{
    (static_cast<aio_queue *>(arg))->post_requests();
    return NULL;
}

void * aio_queue::wait_async(void * arg)
{
    (static_cast<aio_queue *>(arg))->wait_requests();
    return NULL;
}

__STXXL_END_NAMESPACE

#endif // #if STXXL_HAVE_AIO_FILE
// vim: et:ts=4:sw=4
