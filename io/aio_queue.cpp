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
    num_waiting_requests(0), num_free_events(max_sim_requests), num_posted_requests(0),
    post_thread_state(NOT_RUNNING), wait_thread_state(NOT_RUNNING)
{
    if (max_sim_requests == 0)
        max_events = std::max(config::get_instance()->disks_number(), 4u) * 64; // 64 entries per disk should be enough
    else
        max_events = max_sim_requests;

    // negotiate maximum number of simultaneous events with the OS
    context = 0;
    int result;
    while ((result = syscall(SYS_io_setup, max_events, &context)) == -1 && errno == EAGAIN && max_events > 1)
        max_events <<= 1;               // try with half as many events
    if (result != 0)
        STXXL_THROW2(io_error, "io_setup() nr_events=" << max_events);

    STXXL_VERBOSE1("Set up an aio queue with " << max_events << " entries.");

    start_thread(post_async, static_cast<void *>(this), post_thread, post_thread_state);
    start_thread(wait_async, static_cast<void *>(this), wait_thread, wait_thread_state);
}

aio_queue::~aio_queue()
{
    stop_thread(post_thread, post_thread_state, num_waiting_requests);
    stop_thread(wait_thread, wait_thread_state, num_posted_requests);
    syscall(SYS_io_destroy, context);
}

void aio_queue::add_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (post_thread_state() != RUNNING)
        STXXL_ERRMSG("Request submitted to stopped queue.");

    scoped_mutex_lock lock(waiting_mtx);

    waiting_requests.push_back(req);
    num_waiting_requests++;
}

bool aio_queue::cancel_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (post_thread_state() != RUNNING)
        STXXL_ERRMSG("Request canceled in stopped queue.");

    bool was_still_in_queue = false;
    queue_type::iterator pos;
    {
        scoped_mutex_lock lock(waiting_mtx);
        if ((pos = std::find(waiting_requests.begin(), waiting_requests.end(), req _STXXL_FORCE_SEQUENTIAL)) != waiting_requests.end())
        {
            waiting_requests.erase(pos);
            was_still_in_queue = true;
            num_waiting_requests--;  // will never block
        }
    }
    if (!was_still_in_queue)
        return req->cancel();
    return was_still_in_queue;
}

// internal routines, run by the posting thread
void aio_queue::post_requests()
{
    request_ptr req;
    io_event * events = new io_event[max_events];

    for ( ; ; )
    {
        int num_currently_waiting_requests = num_waiting_requests--;    // might block until next request or message comes in

        // terminate if termination has been requested
        if (post_thread_state() == TERMINATE && num_currently_waiting_requests == 0)
            break;

        scoped_mutex_lock lock(waiting_mtx);
        if (!waiting_requests.empty())
        {
            req = waiting_requests.front();
            waiting_requests.pop_front();
            lock.unlock();

            if (max_sim_requests != 0)  //unless unlimited
                num_free_events--;  //might block because too many requests are posted

            while (!static_cast<aio_request *>(req.get())->post())
            {   // post failed, so first handle events to make queues (more) empty
                int num_events = syscall(SYS_io_getevents, context, 1, max_events, events, NULL);
                if (num_events < 0)
                    STXXL_THROW2(io_error, "io_getevents() nr_events=" << num_events);

                handle_events(events, num_events, false);
            }

            // request is finally posted

            {
                scoped_mutex_lock lock(posted_mtx);
                posted_requests.push_back(req);
                num_posted_requests++;
            }
        }
        else
        {
            lock.unlock();

            num_waiting_requests++;  // num_waiting_requests-- was premature, compensate for that
        }
    }

    delete[] events;
}

void aio_queue::handle_events(io_event * events, int num_events, bool canceled)
{
    for (int e = 0; e < num_events; ++e)
    {
        // unsigned_type is as long as a pointer, and like this, we avoid an icpc warning
        request_ptr * r = reinterpret_cast<request_ptr *>(static_cast<unsigned_type>(events[e].data));
        static_cast<aio_request *>(r->get())->completed(canceled);
        delete r;         // release auto_ptr reference
        if (max_sim_requests != 0)
            num_free_events++;
        num_posted_requests--;
    }
}

// internal routines, run by the waiting thread
void aio_queue::wait_requests()
{
    request_ptr req;
    io_event * events = new io_event[max_events];

    for ( ; ; )
    {
        int num_currently_posted_requests = num_posted_requests--;

        // terminate if termination has been requested
        if (wait_thread_state() == TERMINATE && num_currently_posted_requests == 0)
            break;

        // wait for at least one of them to finish
        int num_events = syscall(SYS_io_getevents, context, 1, max_events, events, NULL);
        if (num_events < 0)
            STXXL_THROW2(io_error, "io_getevents() nr_events=" << max_events);

        num_posted_requests++;           // compensate for the one eaten prematurely above

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
