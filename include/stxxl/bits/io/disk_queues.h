/***************************************************************************
 *  include/stxxl/bits/io/disk_queues.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_DISK_QUEUES_HEADER
#define STXXL_IO_DISK_QUEUES_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <queue>
#include <map>

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
 #include <boost/thread/thread.hpp>
#else
 #include <pthread.h>
#endif

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/semaphore.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

typedef stxxl::int64 DISKID;

class disk_queue : private noncopyable
{
public:
    enum priority_op { READ, WRITE, NONE };

private:
    mutex write_mutex;
    mutex read_mutex;
    std::queue<request_ptr> write_queue;
    std::queue<request_ptr> read_queue;

    semaphore sem;

    static const priority_op _priority_op = WRITE;

#ifdef STXXL_BOOST_THREADS
    boost::thread thread;
#else
    pthread_t thread;
#endif

    static void * worker(void * arg);

public:
    disk_queue(int n = 1);             // max number of requests simultaneously submitted to disk

    // in a multi-threaded setup this does not work as intended
    // also there were race conditions possible
    // and actually an old value was never restored once a new one was set ...
    // so just disable it and all it's nice implications
    void set_priority_op(priority_op op)
    {
        //_priority_op = op;
        UNUSED(op);
    }
    void add_readreq(request_ptr & req);
    void add_writereq(request_ptr & req);
    ~disk_queue();
};

//! \brief Encapsulates disk queues
//! \remark is a singleton
class disk_queues : public singleton<disk_queues>
{
    friend class singleton<disk_queues>;

protected:
    std::map<DISKID, disk_queue *> queues;
    disk_queues() { }

public:
    void add_readreq(request_ptr & req, DISKID disk)
    {
        if (queues.find(disk) == queues.end())
        {
            // create new disk queue
            queues[disk] = new disk_queue();
        }
        queues[disk]->add_readreq(req);
    }
    void add_writereq(request_ptr & req, DISKID disk)
    {
        if (queues.find(disk) == queues.end())
        {
            // create new disk queue
            queues[disk] = new disk_queue();
        }
        queues[disk]->add_writereq(req);
    }
    ~disk_queues()
    {
        // deallocate all queues
        for (std::map<DISKID, disk_queue *>::iterator i =
                 queues.begin(); i != queues.end(); i++)
            delete (*i).second;
    }
    //! \brief Changes requests priorities
    //! \param op one of:
    //!                 - READ, read requests are served before write requests within a disk queue
    //!                 - WRITE, write requests are served before read requests within a disk queue
    //!                 - NONE, read and write requests are served by turns, alternately
    void set_priority_op(disk_queue::priority_op op)
    {
        for (std::map<DISKID, disk_queue *>::iterator i =
                 queues.begin(); i != queues.end(); i++)
            i->second->set_priority_op(op);
    }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_DISK_QUEUES_HEADER
// vim: et:ts=4:sw=4
