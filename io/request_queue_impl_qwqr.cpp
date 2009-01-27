/***************************************************************************
 *  io/diskqueue.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2005 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/request_queue_impl_qwqr.h>
#include <stxxl/bits/io/request.h>

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
 #include <boost/bind.hpp>
#endif


__STXXL_BEGIN_NAMESPACE

request_queue_impl_qwqr::request_queue_impl_qwqr(int /*n*/) :              //  n is ignored
    _thread_state(RUNNING), sem(0)
#ifdef STXXL_BOOST_THREADS
                                    , thread(boost::bind(worker, static_cast<void *>(this)))
#endif
{
    //  cout << "disk_queue created." << endl;

#ifdef STXXL_BOOST_THREADS
    // nothing to do
#else
    check_pthread_call(pthread_create(&thread, NULL,
                                      worker, static_cast<void *>(this)));
#endif
}

void request_queue_impl_qwqr::add_request(request_ptr & req)
{
    if (req.empty())
        STXXL_THROW_INVALID_ARGUMENT("Empty request submitted to disk_queue.");
    if (_thread_state() != RUNNING)
        STXXL_THROW_INVALID_ARGUMENT("Request submitted to not running queue.");

    if (req.get()->get_type() == request::READ)
    {
        scoped_mutex_lock Lock(read_mutex);
        read_queue.push(req);
    }
    else
    {
        scoped_mutex_lock Lock(write_mutex);
        write_queue.push(req);
    }

    sem++;
}

request_queue_impl_qwqr::~request_queue_impl_qwqr()
{
    _thread_state.set_to(TERMINATE);
    sem++;
#ifdef STXXL_BOOST_THREADS
    // FIXME: how can boost wait for thread termination?
#else
    check_pthread_call(pthread_join(thread, NULL));
#endif
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
                pthis->write_queue.pop();

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
                pthis->read_queue.pop();

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
