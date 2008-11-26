/***************************************************************************
 *  include/stxxl/bits/io/request.h
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

#ifndef STXXL_REQUEST_HEADER
#define STXXL_REQUEST_HEADER

#include <iostream>
#include <memory>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/io/iostats.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/io/completion_handler.h>
#include <stxxl/bits/compat_unique_ptr.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

#define BLOCK_ALIGN 4096

class file;
class request_ptr;

//! \brief Defines interface of request

//! Since all library I/O operations are asynchronous,
//! one needs to keep track of their status: whether
//! an I/O completed or not.
class request_base : private noncopyable
{
public:
    enum request_type { READ, WRITE };

protected:
    virtual bool add_waiter(onoff_switch * sw) = 0;
    virtual void delete_waiter(onoff_switch * sw) = 0;
    virtual void notify_waiters() = 0;

    virtual void serve() = 0;

public:
    //! \brief Suspends calling thread until completion of the request
    virtual void wait() = 0;

    //! \brief Polls the status of the request
    //! \return \c true if request is completed, otherwise \c false
    virtual bool poll() = 0;

    //! \brief Identifies the type of I/O implementation
    //! \return pointer to null terminated string of characters, containing the name of I/O implementation
    virtual const char * io_type() const = 0;

    virtual ~request_base()
    { }
};

class request : virtual public request_base
{
    friend int wait_any(request_ptr req_array[], int count);
    template <class request_iterator_>
    friend
    request_iterator_ wait_any(request_iterator_ reqs_begin, request_iterator_ reqs_end);
    friend class request_queue_impl_qwqr;
    friend class request_ptr;

protected:
    completion_handler on_complete;
    int ref_cnt;
    compat_unique_ptr<stxxl::io_error>::result error;

    mutex ref_cnt_mutex;

protected:
    file * file_;
    void * buffer;
    stxxl::int64 offset;
    size_t bytes;
    request_type type;

    void completed()
    {
        notify_waiters();
        on_complete(this);
    }

    // returns number of references
    int nref()
    {
        scoped_mutex_lock Lock(ref_cnt_mutex);
        return ref_cnt;
    }

public:
    request(const completion_handler & on_compl,
            file * file__,
            void * buffer_,
            stxxl::int64 offset_,
            size_t bytes_,
            request_type type_);

    virtual ~request();

    file * get_file() const { return file_; }
    void * get_buffer() const { return buffer; }
    stxxl::int64 get_offset() const { return offset; }
    size_t get_size() const { return bytes; }
    request_type get_type() const { return type; }

    void check_alignment() const;

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
    void add_ref()
    {
        scoped_mutex_lock Lock(ref_cnt_mutex);
        ref_cnt++;
        STXXL_VERBOSE3("request add_ref() " << static_cast<void *>(this) << ": adding reference, cnt: " << ref_cnt);
    }

    bool sub_ref()
    {
        int val;
        {
            scoped_mutex_lock Lock(ref_cnt_mutex);
            val = --ref_cnt;
            STXXL_VERBOSE3("request sub_ref() " << static_cast<void *>(this) << ": subtracting reference cnt: " << ref_cnt);
        }
        assert(val >= 0);
        return (val == 0);
    }

protected:
    void check_nref(bool after = false)
    {
        if (nref() < 2)
            check_nref_failed(after);
    }

private:
    void check_nref_failed(bool after = false);
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
    request_ptr & operator = (const request_ptr & p)
    {
        // assert(p.ptr);
        return (*this = p.ptr);
    }
    //! \brief Assignment operator from \c request pointer
    //! \return reference to itself
    request_ptr & operator = (request * p)
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
inline bool poll_any(request_ptr req_array[], int count, int & index);


void wait_all(request_ptr req_array[], int count)
{
    for (int i = 0; i < count; i++)
    {
        req_array[i]->wait();
    }
}

template <class request_iterator_>
void wait_all(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    while (reqs_begin != reqs_end)
    {
        (request_ptr(*reqs_begin))->wait();
        ++reqs_begin;
    }
}

bool poll_any(request_ptr req_array[], int count, int & index)
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


int wait_any(request_ptr req_array[], int count)
{
    stats::scoped_wait_timer wait_timer;

    onoff_switch sw;
    int i = 0, index = -1;

    for ( ; i < count; i++)
    {
        if (req_array[i]->add_waiter(&sw))
        {
            // already done
            index = i;

            while (--i >= 0)
                req_array[i]->delete_waiter(&sw);

            req_array[index]->check_errors();

            return index;
        }
    }

    sw.wait_for_on();

    for (i = 0; i < count; i++)
    {
        req_array[i]->delete_waiter(&sw);
        if (index < 0 && req_array[i]->poll())
            index = i;
    }

    return index;
}

template <class request_iterator_>
request_iterator_ wait_any(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    stats::scoped_wait_timer wait_timer;

    onoff_switch sw;

    request_iterator_ cur = reqs_begin, result = reqs_end;

    for ( ; cur != reqs_end; cur++)
    {
        if ((request_ptr(*cur))->add_waiter(&sw))
        {
            // already done
            result = cur;

            if (cur != reqs_begin)
            {
                while (--cur != reqs_begin)
                    (request_ptr(*cur))->delete_waiter(&sw);

                (request_ptr(*cur))->delete_waiter(&sw);
            }

            (request_ptr(*result))->check_errors();

            return result;
        }
    }

    sw.wait_for_on();

    for (cur = reqs_begin; cur != reqs_end; cur++)
    {
        (request_ptr(*cur))->delete_waiter(&sw);
        if (result == reqs_end && (request_ptr(*cur))->poll())
            result = cur;
    }

    return result;
}

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_REQUEST_HEADER
// vim: et:ts=4:sw=4
