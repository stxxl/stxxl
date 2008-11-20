/***************************************************************************
 *  include/stxxl/bits/io/iobase.h
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

#ifndef STXXL_IOBASE_HEADER
#define STXXL_IOBASE_HEADER

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#if defined (__linux__)
 #define STXXL_CHECK_BLOCK_ALIGNING
#endif

//#ifdef __sun__
//#define O_DIRECT 0
//#endif


#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <algorithm>
#include <string>
#include <queue>
#include <map>
#include <set>

#ifdef BOOST_MSVC
// this is not stxxl/bits/io/io.h !
 #include <io.h>
#else
 #include <unistd.h>
 #include <sys/resource.h>
 #include <sys/wait.h>
#endif

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
 #include <boost/thread/thread.hpp>
#else
 #include <pthread.h>
#endif


#ifndef O_SYNC
 #define O_SYNC 0
#endif
#ifndef O_RSYNC
 #define O_RSYNC 0
#endif
#ifndef O_DSYNC
 #define O_DSYNC 0
#endif


#if defined (__linux__)
//#include <asm/fcntl.h>
// FIXME: In which conditions is this not defined? Why only i386 and alpha? Why not amd64?
 #if !defined (O_DIRECT) && (defined (__alpha__) || defined (__i386__))
  #define O_DIRECT 040000       /* direct disk access */
 #endif
#endif


#ifndef O_DIRECT
 #define O_DIRECT O_SYNC
#endif


#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/semaphore.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/state.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/io/completion_handler.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \defgroup iolayer I/O primitives layer
//! Group of classes which enable abstraction from operating system calls and support
//! system-independent interfaces for asynchronous I/O.
//! \{

#define BLOCK_ALIGN 4096

typedef stxxl::int64 DISKID;

class disk_queue;

//! \brief Default completion handler class

struct default_completion_handler
{
    //! \brief An operator that does nothing
    void operator () (request *) { }
};

//! \brief Defines interface of file

//! It is a base class for different implementations that might
//! base on various file systems or even remote storage interfaces
class file : private noncopyable
{
    int id;

protected:
    //! \brief Initializes file object
    //! \param _id file identifier
    //! \remark Called in implementations of file
    file(int _id) : id(_id) { }

public:
    //! \brief Definition of acceptable file open modes

    //! Various open modes in a file system must be
    //! converted to this set of acceptable modes
    enum open_mode
    {
        RDONLY = 1,                         //!< only reading of the file is allowed
        WRONLY = 2,                         //!< only writing of the file is allowed
        RDWR = 4,                           //!< read and write of the file are allowed
        CREAT = 8,                          //!< in case file does not exist no error occurs and file is newly created
        DIRECT = 16,                        //!< I/Os proceed bypassing file system buffers, i.e. unbuffered I/O
        TRUNC = 32                          //!< once file is opened its length becomes zero
    };

    //! \brief Schedules asynchronous read request to the file
    //! \param buffer pointer to memory buffer to read into
    //! \param pos starting file position to read
    //! \param bytes number of bytes to transfer
    //! \param on_cmpl I/O completion handler
    //! \return \c request_ptr object, that can be used to track the status of the operation
    virtual request_ptr aread(void * buffer, stxxl::int64 pos, size_t bytes,
                              completion_handler on_cmpl) = 0;
    //! \brief Schedules asynchronous write request to the file
    //! \param buffer pointer to memory buffer to write from
    //! \param pos starting file position to write
    //! \param bytes number of bytes to transfer
    //! \param on_cmpl I/O completion handler
    //! \return \c request_ptr object, that can be used to track the status of the operation
    virtual request_ptr awrite(void * buffer, stxxl::int64 pos, size_t bytes,
                               completion_handler on_cmpl) = 0;

    virtual void serve(const request * req) throw(io_error) = 0;

    //! \brief Changes the size of the file
    //! \param newsize value of the new file size
    virtual void set_size(stxxl::int64 newsize) = 0;
    //! \brief Returns size of the file
    //! \return file size in bytes
    virtual stxxl::int64 size() = 0;
    //! \brief deprecated, use \c stxxl::file::get_id() instead
    __STXXL_DEPRECATED(int get_disk_number() const)
    {
        return get_id();
    }
    //! \brief Returns file's identifier
    //! \remark might be used as disk's id in case disk to file mapping
    //! \return integer file identifier, passed as constructor parameter
    int get_id() const
    {
        return id;
    }

    //! \brief Locks file for reading and writing (aquires a lock in the file system)
    virtual void lock() = 0;

    //! \brief Some specialized file types may need to know freed regions
    virtual void delete_region(int64 offset, unsigned_type size)
    {
        UNUSED(offset);
        UNUSED(size);
    }

    virtual ~file() { }
};


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

#endif // !STXXL_IOBASE_HEADER
// vim: et:ts=4:sw=4
