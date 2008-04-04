#ifndef IOBASE_HEADER
#define IOBASE_HEADER

/***************************************************************************
 *            iobase.h
 *
 *  Sat Aug 24 23:54:44 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#define STXXL_IO_STATS


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
 #include <io.h>
#else
 #include <unistd.h>
 #include <sys/resource.h>
 #include <sys/wait.h>
#endif

#ifdef STXXL_BOOST_THREADS // Use Portable Boost threads
// Boost.Threads headers
 #include <boost/thread/thread.hpp>
 #include <boost/thread/mutex.hpp>
 #include <boost/bind.hpp>
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
 #if !defined (O_DIRECT) && (defined (__alpha__) || defined (__i386__))
  #define O_DIRECT 040000       /* direct disk access */
 #endif
#endif


#ifndef O_DIRECT
 #define O_DIRECT O_SYNC
#endif


#include "stxxl/bits/namespace.h"
#include "stxxl/bits/io/iostats.h"
#include "stxxl/bits/common/semaphore.h"
#include "stxxl/bits/common/mutex.h"
#include "stxxl/bits/common/switch.h"
#include "stxxl/bits/common/state.h"
#include "stxxl/bits/common/exceptions.h"
#include "stxxl/bits/io/completion_handler.h"


__STXXL_BEGIN_NAMESPACE

//! \defgroup iolayer I/O primitives layer
//! Group of classes which enable abstraction from operating system calls and support
//! system-independent interfaces for asynchronous I/O.
//! \{

#define BLOCK_ALIGN 4096

typedef void * (*thread_function_t)(void *);
typedef stxxl::int64 DISKID;

class request;
class request_ptr;

//! \brief Default completion handler class

struct default_completion_handler
{
    //! \brief An operator that does nothing
    void operator()  (request *) { }
};

//! \brief Defines interface of file

//! It is a base class for different implementations that might
//! base on various file systems or even remote storage interfaces
class file : private noncopyable
{
    //! \brief private constructor
    //! \remark instantiation of file without id is forbidden
    file ()
    { };

protected:
    int id;

    //! \brief Initializes file object
    //! \param _id file identifier
    //! \remark Called in implementations of file
    file (int _id) : id (_id) { };

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
    virtual request_ptr aread (void * buffer, stxxl::int64 pos, size_t bytes,
                               completion_handler on_cmpl ) = 0;
    //! \brief Schedules asynchronous write request to the file
    //! \param buffer pointer to memory buffer to write from
    //! \param pos starting file position to write
    //! \param bytes number of bytes to transfer
    //! \param on_cmpl I/O completion handler
    //! \return \c request_ptr object, that can be used to track the status of the operation
    virtual request_ptr awrite (void * buffer, stxxl::int64 pos, size_t bytes,
                                completion_handler on_cmpl ) = 0;

    //! \brief Changes the size of the file
    //! \param newsize value of the new file size
    virtual void set_size (stxxl::int64 newsize) = 0;
    //! \brief Returns size of the file
    //! \return file size in bytes
    virtual stxxl::int64 size () = 0;
    //! \brief deprecated, use \c stxxl::file::get_id() instead
    int get_disk_number ()
    {
        return id;
    }
    //! \brief Returns file's identifier
    //! \remark might be used as disk's id in case disk to file mapping
    //! \return integer file identifier, passed as constructor parameter
    int get_id()
    {
        return id;
    }

    //! \brief Locks file for reading and writing
    virtual void lock() { }

    virtual ~file () { }
};

class mc;
class disk_queue;
class disk_queues;

//! \brief Defines interface of request

//! Since all library I/O operations are asynchronous,
//! one needs to keep track of their status: whether
//! an I/O completed or not.
class request : private noncopyable
{
    friend int wait_any(request_ptr req_array[], int count);
    template <class request_iterator_> friend
    request_iterator_ wait_any(request_iterator_ reqs_begin, request_iterator_ reqs_end);
    friend class file;
    friend class disk_queue;
    friend class disk_queues;
    friend class request_ptr;

protected:
    virtual bool add_waiter (onoff_switch * sw) = 0;
    virtual void delete_waiter (onoff_switch * sw) = 0;
    //virtual void enqueue () = 0;
    virtual void serve () = 0;
    //virtual unsigned size() const;

    completion_handler on_complete;
    int ref_cnt;
    std::auto_ptr<stxxl::io_error> error;

#ifdef STXXL_BOOST_THREADS
    boost::mutex ref_cnt_mutex;
#else
    mutex ref_cnt_mutex;
#endif

public:
    enum request_type { READ, WRITE };

protected:
    file * file_;
    void * buffer;
    stxxl::int64 offset;
    size_t bytes;
    request_type type;

    void completed ()
    {
        on_complete(this);
    }

    // returns number of references
    int nref()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(ref_cnt_mutex);
        return ref_cnt;
#else
        ref_cnt_mutex.lock();
        int ref_cnt_ = ref_cnt;
        ref_cnt_mutex.unlock();
        return ref_cnt_;
#endif
    }

public:
    request(        completion_handler on_compl,
                    file * file__,
                    void * buffer_,
                    stxxl::int64 offset_,
                    size_t bytes_,
                    request_type type_) :
        on_complete(on_compl), ref_cnt(0),
        file_(file__),
        buffer(buffer_),
        offset(offset_),
        bytes(bytes_),
        type(type_)
    {
        STXXL_VERBOSE3("request " << static_cast<void *>(this) << ": creation, cnt: " << ref_cnt);
    }
    //! \brief Suspends calling thread until completion of the request
    virtual void wait () = 0;
    //! \brief Polls the status of the request
    //! \return \c true if request is completed, otherwise \c false
    virtual bool poll () = 0;
    //! \brief Identifies the type of request I/O implementation
    //! \return pointer to null terminated string of characters, containing the name of I/O implementation
    virtual const char * io_type ()
    {
        return "none";
    }
    virtual ~request()
    {
        STXXL_VERBOSE3("request " << static_cast<void *>(this) << ": deletion, cnt: " << ref_cnt);
    }
    file * get_file() const { return file_; }
    void * get_buffer() const { return buffer; }
    stxxl::int64 get_offset() const { return offset; }
    size_t get_size() const { return bytes; }
    size_t size() const { return bytes; }
    request_type get_type() const { return type; }

    virtual std::ostream & print(std::ostream & out) const
    {
        out << "File object address: " << (void *)get_file();
        out << " Buffer address: " << (void *)get_buffer();
        out << " File offset: " << get_offset();
        out << " Transfer size: " << get_size() << " bytes";
        out << " Type of transfer: " << ((get_type() == READ) ? "READ" : "WRITE");
        return out;
    }

    //! \brief Inform the request object that an error occurred
    //! during the I/O execution
    void error_occured(const char * msg)
    {
        error.reset(new stxxl::io_error(msg));
    }

    //! \brief Inform the request object that an error occurred
    //! during the I/O execution
    void error_occured(const std::string & msg)
    {
        error.reset(new stxxl::io_error(msg));
    }

    //! \brief Rises an exception if there were error with the I/O
    void check_errors() throw (stxxl::io_error)
    {
        if (error.get())
            throw * (error.get());
    }

private:
    // Following methods are declared but not implemented
    // intentionally to forbid their usage
    request(const request &);
    request & operator=(const request &);
    request();

    void add_ref()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(ref_cnt_mutex);
        ref_cnt++;
        STXXL_VERBOSE3("request add_ref() " << static_cast<void *>(this) << ": adding reference, cnt: " << ref_cnt);

#else
        ref_cnt_mutex.lock();
        ref_cnt++;
        STXXL_VERBOSE3("request add_ref() " << static_cast<void *>(this) << ": adding reference, cnt: " << ref_cnt);
        ref_cnt_mutex.unlock();
#endif
    }
    bool sub_ref()
    {
#ifdef STXXL_BOOST_THREADS
        boost::mutex::scoped_lock Lock(ref_cnt_mutex);
        int val = --ref_cnt;
        STXXL_VERBOSE3("request sub_ref() " << static_cast<void *>(this) << ": subtracting reference cnt: " << ref_cnt);
        Lock.unlock();

#else
        ref_cnt_mutex.lock();
        int val = --ref_cnt;
        STXXL_VERBOSE3("request sub_ref() " << static_cast<void *>(this) << ": subtracting reference cnt: " << ref_cnt);

        ref_cnt_mutex.unlock();
#endif
        assert(val >= 0);
        return (val == 0);
    }
};

inline std::ostream & operator << (std::ostream & out, const request & req)
{
    return req.print(out);
}

//! \brief A smart wrapper for \c request pointer.

//! Implemented as reference counting smart pointer.
class request_ptr
{
    request * ptr;
    void add_ref()
    {
        if (ptr)
        {
            ptr->add_ref();
        }
    }
    void sub_ref()
    {
        if (ptr)
        {
            if (ptr->sub_ref())
            {
                STXXL_VERBOSE3("the last copy " << static_cast<void *>(ptr) << " this=" << static_cast<void *>(this));
                delete ptr;
                ptr = NULL;
            }
            else
            {
                STXXL_VERBOSE3("more copies " << static_cast<void *>(ptr) << " this=" << static_cast<void *>(this));
            }
        }
    }
public:
    //! \brief Constructs an \c request_ptr from \c request pointer
    request_ptr(request * ptr_ = NULL) : ptr(ptr_)
    {
        STXXL_VERBOSE3("create constructor (request =" << static_cast<void *>(ptr) << ") this=" << static_cast<void *>(this));
        add_ref();
    }
    //! \brief Constructs an \c request_ptr from a \c request_ptr object
    request_ptr(const request_ptr & p) : ptr(p.ptr)
    {
        STXXL_VERBOSE3("copy constructor (copying " << static_cast<void *>(ptr) << ") this=" << static_cast<void *>(this));
        add_ref();
    }
    //! \brief Destructor
    ~request_ptr()
    {
        STXXL_VERBOSE3("Destructor of a request_ptr pointing to " << static_cast<void *>(ptr) << " this=" << static_cast<void *>(this));
        sub_ref();
    }
    //! \brief Assignment operator from \c request_ptr object
    //! \return reference to itself
    request_ptr & operator= (const request_ptr & p)
    {
        // assert(p.ptr);
        return (*this = p.ptr);
    }
    //! \brief Assignment operator from \c request pointer
    //! \return reference to itself
    request_ptr & operator= (request * p)
    {
        STXXL_VERBOSE3("assign operator begin (assigning " << static_cast<void *>(p) << ") this=" << static_cast<void *>(this));
        if (p != ptr)
        {
            sub_ref();
            ptr = p;
            add_ref();
        }
        STXXL_VERBOSE3("assign operator end (assigning " << static_cast<void *>(p) << ") this=" << static_cast<void *>(this));
        return *this;
    }
    //! \brief "Star" operator
    //! \return reference to owned \c request object
    request & operator * () const
    {
        assert(ptr);
        return *ptr;
    }
    //! \brief "Arrow" operator
    //! \return pointer to owned \c request object
    request * operator -> () const
    {
        assert(ptr);
        return ptr;
    }
    //! \brief Access to owned \c request object (synonym for \c operator->() )
    //! \return reference to owned \c request object
    //! \warning Creation another \c request_ptr from the returned \c request or deletion
    //!  causes unpredictable behaviour. Do not do that!
    request * get() const { return ptr; }

    //! \brief Returns true if object is initialized
    bool valid() const { return ptr; }

    //! \brief Returns true if object is not initialized
    bool empty() const { return !ptr; }
};

//! \brief Collection of functions to track statuses of a number of requests

//! \brief Suspends calling thread until \b any of requests is completed
//! \param req_array array of \c request_ptr objects
//! \param count size of req_array
//! \return index in req_array pointing to the \b first completed request
inline int wait_any(request_ptr req_array[], int count);
//! \brief Suspends calling thread until \b all requests are completed
//! \param req_array array of request_ptr objects
//! \param count size of req_array
inline void wait_all(request_ptr req_array[], int count);
//! \brief Polls requests
//! \param req_array array of request_ptr objects
//! \param count size of req_array
//! \param index contains index of the \b first completed request if any
//! \return \c true if any of requests is completed, then index contains valid value, otherwise \c false
inline bool poll_any (request_ptr req_array[], int count, int &index);


void wait_all(request_ptr req_array[], int count)
{
    for (int i = 0; i < count; i++)
    {
        req_array[i]->wait ();
    }
}

template <class request_iterator_>
void wait_all(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    while (reqs_begin != reqs_end)
    {
        (request_ptr(*reqs_begin))->wait ();
        ++reqs_begin;
    }
}

bool poll_any (request_ptr req_array[], int count, int &index)
{
    index = -1;
    for (int i = 0; i < count; i++)
    {
        if (req_array[i]->poll())
        {
            index = i;
            return true;
        }
    }
    return false;
}

template <class request_iterator_>
request_iterator_ poll_any(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    while (reqs_begin != reqs_end)
    {
        if ((request_ptr(*reqs_begin))->poll())
            return reqs_begin;

        ++reqs_begin;
    }
    return reqs_end;
}


int wait_any (request_ptr req_array[], int count)
{
    START_COUNT_WAIT_TIME
    onoff_switch sw;
    int i = 0, index = -1;

    for ( ; i < count; i++)
    {
        if (req_array[i]->add_waiter (&sw))
        {
            // already done
            index = i;

            while (--i >= 0)
                req_array[i]->delete_waiter (&sw);


            END_COUNT_WAIT_TIME

            req_array[index]->check_errors();

            return index;
        }
    }

    sw.wait_for_on ();

    for (i = 0; i < count; i++)
    {
        req_array[i]->delete_waiter (&sw);
        if (index < 0 && req_array[i]->poll ())
            index = i;
    }

    END_COUNT_WAIT_TIME
    return index;
}

template <class request_iterator_>
request_iterator_ wait_any(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    START_COUNT_WAIT_TIME
    onoff_switch sw;

    request_iterator_ cur = reqs_begin, result = reqs_end;

    for ( ; cur != reqs_end; cur++)
    {
        if ((request_ptr(*cur))->add_waiter (&sw))
        {
            // already done
            result = cur;

            if (cur != reqs_begin)
            {
                while (--cur != reqs_begin)
                    (request_ptr(*cur))->delete_waiter (&sw);

                (request_ptr(*cur))->delete_waiter (&sw);
            }

            END_COUNT_WAIT_TIME

            (request_ptr(*result))->check_errors();

            return result;
        }
    }

    sw.wait_for_on ();

    for (cur = reqs_begin; cur != reqs_end; cur++)
    {
        (request_ptr(*cur))->delete_waiter (&sw);
        if (result == reqs_end && (request_ptr(*cur))->poll ())
            result = cur;
    }

    END_COUNT_WAIT_TIME
    return result;
}

class disk_queue : private noncopyable
{
public:
    enum priority_op { READ, WRITE, NONE };
private:
#ifdef STXXL_BOOST_THREADS
    boost::mutex write_mutex;
    boost::mutex read_mutex;
#else
    mutex write_mutex;
    mutex read_mutex;
#endif
    std::queue < request_ptr > write_queue;
    std::queue < request_ptr > read_queue;

    semaphore sem;

    priority_op _priority_op;

#ifdef STXXL_BOOST_THREADS
    boost::thread thread;
#else
    pthread_t thread;
#endif


#ifdef STXXL_IO_STATS
    stats * iostats;
#endif

    static void * worker (void * arg);
public:
    disk_queue (int n = 1);             // max number of requests simultaneously submitted to disk

    void set_priority_op (priority_op op)
    {
        _priority_op = op;
    }
    void add_readreq (request_ptr & req);
    void add_writereq (request_ptr & req);
    ~disk_queue ();
};

//! \brief Encapsulates disk queues
//! \remark is a singleton
class disk_queues : private noncopyable
{
protected:
    std::map < DISKID, disk_queue * > queues;
    disk_queues () { }
public:
    void add_readreq (request_ptr & req, DISKID disk)
    {
        if (queues.find (disk) == queues.end ())
        {
            // create new disk queue
            queues[disk] = new disk_queue ();
        }
        queues[disk]->add_readreq (req);
    }
    void add_writereq (request_ptr & req, DISKID disk)
    {
        if (queues.find (disk) == queues.end ())
        {
            // create new disk queue
            queues[disk] = new disk_queue ();
        }
        queues[disk]->add_writereq (req);
    }
    ~disk_queues ()
    {
        // deallocate all queues
        for (std::map < DISKID, disk_queue * > ::iterator i =
                 queues.begin (); i != queues.end (); i++)
            delete (*i).second;
    };
    static disk_queues * get_instance ()
    {
        if (!instance)
            instance = new disk_queues ();


        return instance;
    }
    //! \brief Changes requests priorities
    //! \param op one of:
    //!                 - READ, read requests are served before write requests within a disk queue
    //!                 - WRITE, write requests are served before read requests within a disk queue
    //!                 - NONE, read and write requests are served by turns, alternately
    void set_priority_op(disk_queue::priority_op op)
    {
        for (std::map < DISKID, disk_queue * > ::iterator i =
                 queues.begin (); i != queues.end (); i++)
            i->second->set_priority_op (op);
    }
private:
    static disk_queues * instance;
};

//! \}

__STXXL_END_NAMESPACE


#endif
