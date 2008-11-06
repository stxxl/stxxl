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

#include <stxxl/bits/io/iobase.h>

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
 #include <boost/bind.hpp>
#endif


__STXXL_BEGIN_NAMESPACE

disk_queue::disk_queue(int /*n*/) : sem(0), _priority_op(WRITE)              //  n is ignored
#ifdef STXXL_BOOST_THREADS
                                    , thread(boost::bind(worker, static_cast<void *>(this)))
#endif
{
    //  cout << "disk_queue created." << endl;

#ifdef STXXL_BOOST_THREADS
    // nothing to do
#else
    check_pthread_call(pthread_create(&thread, NULL,
                                      (thread_function_t)worker, static_cast<void *>(this)));
#endif
}

void disk_queue::add_readreq(request_ptr & req)
{
    {
        scoped_mutex_lock Lock(read_mutex);
        read_queue.push(req);
    }

    sem++;
}

void disk_queue::add_writereq(request_ptr & req)
{
    {
        scoped_mutex_lock Lock(write_mutex);
        write_queue.push(req);
    }

    sem++;
}

disk_queue::~disk_queue()
{
#ifdef STXXL_BOOST_THREADS
    // Boost.Threads do not support cancellation ?
#else
    check_pthread_call(pthread_cancel(thread));
    check_pthread_call(pthread_join(thread, NULL));
#endif
}

void * disk_queue::worker(void * arg)
{
    disk_queue * pthis = static_cast<disk_queue *>(arg);
    request_ptr req;

#ifdef STXXL_BOOST_THREADS
#else
    check_pthread_call(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
    check_pthread_call(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));
    // Allow cancellation in semaphore operator-- call
#endif

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
    }

    return NULL;
}

__STXXL_END_NAMESPACE
