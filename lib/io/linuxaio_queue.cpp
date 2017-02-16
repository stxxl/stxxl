/***************************************************************************
 *  lib/io/linuxaio_queue.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *  Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/linuxaio_queue.h>

#if STXXL_HAVE_LINUXAIO_FILE

#include <sys/syscall.h>
#include <unistd.h>

#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/io/linuxaio_queue.h>
#include <stxxl/bits/io/linuxaio_request.h>
#include <stxxl/bits/mng/block_manager.h>
#include <stxxl/bits/verbose.h>

#include <algorithm>

#ifndef STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION
#define STXXL_CHECK_FOR_PENDING_REQUESTS_ON_SUBMISSION 1
#endif

namespace stxxl {

linuxaio_queue::linuxaio_queue(int desired_queue_length)
    : num_waiting_requests_(0), num_free_events_(0), num_posted_requests_(0),
      post_thread_state_(NOT_RUNNING), wait_thread_state_(NOT_RUNNING)
{
    if (desired_queue_length == 0) {
        // default value, 64 entries per queue (i.e. usually per disk) should
        // be enough
        max_events_ = 64;
    }
    else
        max_events_ = desired_queue_length;

    // negotiate maximum number of simultaneous events with the OS
    context_ = 0;
    long result;
    while ((result = syscall(SYS_io_setup, max_events_, &context_)) == -1 &&
           errno == EAGAIN && max_events_ > 1)
    {
        max_events_ <<= 1;               // try with half as many events
    }
    if (result != 0) {
        STXXL_THROW_ERRNO(io_error, "linuxaio_queue::linuxaio_queue"
                          " io_setup() nr_events=" << max_events_);
    }

    num_free_events_.signal(max_events_);

    STXXL_MSG("Set up an linuxaio queue with " << max_events_ << " entries.");

    start_thread(post_async, static_cast<void*>(this), post_thread_, post_thread_state_);
    start_thread(wait_async, static_cast<void*>(this), wait_thread_, wait_thread_state_);
}

linuxaio_queue::~linuxaio_queue()
{
    stop_thread(post_thread_, post_thread_state_, num_waiting_requests_);
    stop_thread(wait_thread_, wait_thread_state_, num_posted_requests_);
    syscall(SYS_io_destroy, context_);
}

void linuxaio_queue::add_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (post_thread_state_() != RUNNING)
        STXXL_ERRMSG("Request submitted to stopped queue.");
    if (!dynamic_cast<linuxaio_request*>(req.get()))
        STXXL_ERRMSG("Non-LinuxAIO request submitted to LinuxAIO queue.");

    std::unique_lock<std::mutex> lock(waiting_mtx_);

    waiting_requests_.push_back(req);
    num_waiting_requests_.signal();
}

bool linuxaio_queue::cancel_request(request_ptr& req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request canceled disk_queue.");
    if (post_thread_state_() != RUNNING)
        STXXL_ERRMSG("Request canceled in stopped queue.");
    if (!dynamic_cast<linuxaio_request*>(req.get()))
        STXXL_ERRMSG("Non-LinuxAIO request submitted to LinuxAIO queue.");

    queue_type::iterator pos;
    {
        std::unique_lock<std::mutex> lock(waiting_mtx_);

        pos = std::find(
            waiting_requests_.begin(), waiting_requests_.end(), req);
        if (pos != waiting_requests_.end())
        {
            waiting_requests_.erase(pos);

            // polymorphic_downcast to linuxaio_request,
            // request is canceled, but was not yet posted.
            dynamic_cast<linuxaio_request*>(req.get())->completed(false, true);

            num_waiting_requests_.wait(); // will never block
            return true;
        }
    }

    std::unique_lock<std::mutex> lock(posted_mtx_);

    pos = std::find(posted_requests_.begin(), posted_requests_.end(), req);
    if (pos != posted_requests_.end())
    {
        // polymorphic_downcast to linuxaio_request,
        bool canceled_io_operation =
            (dynamic_cast<linuxaio_request*>(req.get()))->cancel_aio();

        if (canceled_io_operation)
        {
            posted_requests_.erase(pos);

            // polymorphic_downcast to linuxaio_request,

            // request is canceled, already posted
            dynamic_cast<linuxaio_request*>(req.get())->completed(true, true);

            num_free_events_.signal();
            num_posted_requests_.wait(); // will never block
            return true;
        }
    }

    return false;
}

// internal routines, run by the posting thread
void linuxaio_queue::post_requests()
{
    request_ptr req;
    io_event* events = new io_event[max_events_];

    for ( ; ; ) // as long as thread is running
    {
        // might block until next request or message comes in
        int num_currently_waiting_requests = num_waiting_requests_.wait();

        // terminate if termination has been requested
        if (post_thread_state_() == TERMINATING && num_currently_waiting_requests == 0)
            break;

        std::unique_lock<std::mutex> lock(waiting_mtx_);
        if (!waiting_requests_.empty())
        {
            req = waiting_requests_.front();
            waiting_requests_.pop_front();
            lock.unlock();

            num_free_events_.wait(); // might block because too many requests are posted

            // polymorphic_downcast
            while (!dynamic_cast<linuxaio_request*>(req.get())->post())
            {
                // post failed, so first handle events to make queues (more)
                // empty, then try again.

                // wait for at least one event to complete, no time limit
                long num_events = syscall(SYS_io_getevents, context_, 1, max_events_, events, nullptr);
                if (num_events < 0) {
                    STXXL_THROW_ERRNO(io_error, "linuxaio_queue::post_requests"
                                      " io_getevents() nr_events=" << num_events);
                }

                handle_events(events, num_events, false);
            }

            // request is finally posted

            {
                std::unique_lock<std::mutex> lock(posted_mtx_);
                posted_requests_.push_back(req);
                num_posted_requests_.signal();
            }
        }
        else
        {
            lock.unlock();

            // num_waiting_requests_-- was premature, compensate for that
            num_waiting_requests_.signal();
        }
    }

    delete[] events;
}

void linuxaio_queue::handle_events(io_event* events, long num_events, bool canceled)
{
    for (int e = 0; e < num_events; ++e)
    {
        // size_t is as long as a pointer, and like this, we avoid an icpc warning
        request_ptr* r = reinterpret_cast<request_ptr*>(static_cast<size_t>(events[e].data));
        r->get()->completed(canceled);
        delete r;                    // release auto_ptr reference
        num_free_events_.signal();
        num_posted_requests_.wait(); // will never block
    }
}

// internal routines, run by the waiting thread
void linuxaio_queue::wait_requests()
{
    request_ptr req;
    io_event* events = new io_event[max_events_];

    for ( ; ; ) // as long as thread is running
    {
        // might block until next request is posted or message comes in
        int num_currently_posted_requests = num_posted_requests_.wait();

        // terminate if termination has been requested
        if (wait_thread_state_() == TERMINATING && num_currently_posted_requests == 0)
            break;

        // wait for at least one of them to finish
        long num_events;
        while (1) {
            num_events = syscall(SYS_io_getevents, context_, 1, max_events_, events, nullptr);
            if (num_events < 0) {
                if (errno == EINTR) {
                    // io_getevents may return prematurely in case a signal is received
                    continue;
                }

                STXXL_THROW_ERRNO(io_error, "linuxaio_queue::wait_requests"
                                  " io_getevents() nr_events=" << max_events_);
            }
            break;
        }

        num_posted_requests_.signal(); // compensate for the one eaten prematurely above

        handle_events(events, num_events, false);
    }

    delete[] events;
}

void* linuxaio_queue::post_async(void* arg)
{
    (static_cast<linuxaio_queue*>(arg))->post_requests();

    self_type* pthis = static_cast<self_type*>(arg);
    pthis->post_thread_state_.set_to(TERMINATED);

#if STXXL_MSVC >= 1700
    // Workaround for deadlock bug in Visual C++ Runtime 2012 and 2013, see
    // request_queue_impl_worker.cpp. -tb
    ExitThread(nullptr);
#else
    return nullptr;
#endif
}

void* linuxaio_queue::wait_async(void* arg)
{
    (static_cast<linuxaio_queue*>(arg))->wait_requests();

    self_type* pthis = static_cast<self_type*>(arg);
    pthis->wait_thread_state_.set_to(TERMINATED);

#if STXXL_MSVC >= 1700
    // Workaround for deadlock bug in Visual C++ Runtime 2012 and 2013, see
    // request_queue_impl_worker.cpp. -tb
    ExitThread(nullptr);
#else
    return nullptr;
#endif
}

} // namespace stxxl

#endif // #if STXXL_HAVE_LINUXAIO_FILE
// vim: et:ts=4:sw=4
