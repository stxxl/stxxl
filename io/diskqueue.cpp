/***************************************************************************
 *            diskqueue.cpp
 *
 *  Sat Aug 24 23:52:06 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/io/iobase.h"
#include "stxxl/bits/common/gprof.h"


#ifndef STXXL_BOOST_THREADS

 #ifdef STXXL_THREAD_PROFILING
  #define pthread_create gprof_pthread_create
 #endif

#endif

__STXXL_BEGIN_NAMESPACE

disk_queues * disk_queues::instance = NULL;

disk_queue::disk_queue (int /*n*/) : sem (0), _priority_op (WRITE)              //  n is ignored
#ifdef STXXL_BOOST_THREADS
                                     , thread(boost::bind(worker, static_cast<void *>(this)))
#endif
{
    //  cout << "disk_queue created." << endl;
#ifdef STXXL_IO_STATS
    iostats = stats::get_instance ();
#endif

#ifdef STXXL_BOOST_THREADS
    // nothing to do
#else
    stxxl_nassert(pthread_create(&thread, NULL,
                                 (thread_function_t) worker, static_cast<void *>(this)),
                  resource_error);
#endif
}

void disk_queue::add_readreq (request_ptr & req)
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(read_mutex);
    read_queue.push (req);
    Lock.unlock();
#else
    read_mutex.lock ();
    read_queue.push (req);
    read_mutex.unlock ();
#endif

    sem++;
}

void disk_queue::add_writereq (request_ptr & req)
{
#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(write_mutex);
    write_queue.push (req);
    Lock.unlock();
#else
    write_mutex.lock ();
    write_queue.push (req);
    write_mutex.unlock ();
#endif
    sem++;
}

disk_queue::~disk_queue ()
{
#ifdef STXXL_BOOST_THREADS
    // Boost.Threads do not support cancellation ?
#else
    stxxl_nassert (pthread_cancel (thread), resource_error);
#endif
}

void * disk_queue::worker (void * arg)
{
    disk_queue * pthis = static_cast<disk_queue *>(arg);
    request_ptr req;

#ifdef STXXL_BOOST_THREADS
#else
    stxxl_nassert (pthread_setcancelstate
                   (PTHREAD_CANCEL_ENABLE, NULL), resource_error);
    stxxl_nassert (pthread_setcanceltype
                   (PTHREAD_CANCEL_DEFERRED, NULL), resource_error);
    // Allow cancellation in semaphore operator-- call
#endif

    bool write_phase = true;
    for ( ; ;)
    {
        pthis->sem--;

        if (write_phase)
        {
#ifdef STXXL_BOOST_THREADS
            boost::mutex::scoped_lock WriteLock(pthis->write_mutex);
#else
            pthis->write_mutex.lock ();
#endif
            if (!pthis->write_queue.empty ())
            {
                req = pthis->write_queue.front ();
                pthis->write_queue.pop ();

#ifdef STXXL_BOOST_THREADS
                WriteLock.unlock();
#else
                pthis->write_mutex.unlock ();
#endif

                //assert(req->nref() > 1);
                req->serve ();
            }
            else
            {
#ifdef STXXL_BOOST_THREADS
                WriteLock.unlock();
#else
                pthis->write_mutex.unlock ();
#endif

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
#ifdef STXXL_BOOST_THREADS
            boost::mutex::scoped_lock ReadLock(pthis->read_mutex);
#else
            pthis->read_mutex.lock ();
#endif

            if (!pthis->read_queue.empty ())
            {
                req = pthis->read_queue.front ();
                pthis->read_queue.pop ();

#ifdef STXXL_BOOST_THREADS
                ReadLock.unlock();
#else
                pthis->read_mutex.unlock ();
#endif

                STXXL_VERBOSE2( "queue: before serve request has " << req->nref() << " references ");
                //assert(req->nref() > 1);
                req->serve ();
                STXXL_VERBOSE2( "queue: after serve request has " << req->nref() << " references ");
            }
            else
            {
#ifdef STXXL_BOOST_THREADS
                ReadLock.unlock();
#else
                pthis->read_mutex.unlock ();
#endif

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

