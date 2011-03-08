/***************************************************************************
 *  include/stxxl/bits/io/request.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_REQUEST_HEADER
#define STXXL_REQUEST_HEADER

#include <iostream>
#include <memory>
#include <cassert>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/common/types.h>
#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/io/iostats.h>
#include <stxxl/bits/common/mutex.h>
#include <stxxl/bits/common/switch.h>
#include <stxxl/bits/common/exceptions.h>
#include <stxxl/bits/io/completion_handler.h>
#include <stxxl/bits/compat_unique_ptr.h>
#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/verbose.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

#define BLOCK_ALIGN 4096

class file;
class request_ptr;

//! \brief Basic properties of a request.
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
    offset_type offset;
    size_type bytes;
    request_type type;

    void completed();

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
            offset_type offset_,
            size_type bytes_,
            request_type type_);

    virtual ~request();

    file * get_file() const { return file_; }
    void * get_buffer() const { return buffer; }
    offset_type get_offset() const { return offset; }
    size_type get_size() const { return bytes; }
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
        STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request::add_ref(): added reference, ref_cnt=" << ref_cnt);
    }

    bool sub_ref()
    {
        int val;
        {
            scoped_mutex_lock Lock(ref_cnt_mutex);
            val = --ref_cnt;
            STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request::sub_ref(): subtracted reference, ref_cnt=" << ref_cnt);
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

#define STXXL_VERBOSE_request_ptr(msg) STXXL_VERBOSE3("[" << static_cast<void *>(this) << "] request_ptr::" << msg << " ptr=" << static_cast<void *>(ptr))

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
                STXXL_VERBOSE_request_ptr("sub_ref(): the last ref, deleting");
                delete ptr;
                ptr = NULL;
            }
            else
            {
                STXXL_VERBOSE_request_ptr("sub_ref(): more refs left");
            }
        }
    }

public:
    //! \brief Constructs an \c request_ptr from \c request pointer
    request_ptr(request * ptr_ = NULL) : ptr(ptr_)
    {
        STXXL_VERBOSE_request_ptr("(request*)");
        add_ref();
    }
    //! \brief Constructs an \c request_ptr from a \c request_ptr object
    request_ptr(const request_ptr & p) : ptr(p.ptr)
    {
        STXXL_VERBOSE_request_ptr("(request_ptr&)");
        add_ref();
    }
    //! \brief Destructor
    ~request_ptr()
    {
        STXXL_VERBOSE_request_ptr("~()");
        sub_ref();
    }
    //! \brief Assignment operator from \c request_ptr object
    //! \return reference to itself
    request_ptr & operator = (const request_ptr & p)
    {
        // assert(p.ptr);
        return (*this = p.ptr); //call the operator below;
    }
    //! \brief Assignment operator from \c request pointer
    //! \return reference to itself
    request_ptr & operator = (request * p)
    {
        STXXL_VERBOSE_request_ptr("operator=(request=" << static_cast<void *>(p) << ") {BEGIN}");
        if (p != ptr)
        {
            sub_ref();
            ptr = p;
            add_ref();
        }
        STXXL_VERBOSE_request_ptr("operator=(request=" << static_cast<void *>(p) << ") {END}");
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

    bool operator == (const request_ptr & rp2) const
    {
        return ptr == rp2.ptr;
    }

    //! \brief Access to owned \c request object (synonym for \c operator->() )
    //! \return reference to owned \c request object
    //! \warning Creation another \c request_ptr from the returned \c request or deletion
    //!  causes unpredictable behaviour. Do not do that!
    request * get() const { return ptr; }

    //! \brief Returns true if object is initialized
    bool valid() const { return ptr != NULL; }

    //! \brief Returns true if object is not initialized
    bool empty() const { return ptr == NULL; }
};

//! \brief Collection of functions to track statuses of a number of requests


//! \brief Suspends calling thread until \b all given requests are completed
//! \param reqs_begin begin of request sequence to wait for
//! \param reqs_end end of request sequence to wait for
template <class request_iterator_>
void wait_all(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    for ( ; reqs_begin != reqs_end; ++reqs_begin)
        (request_ptr(*reqs_begin))->wait();
}

//! \brief Suspends calling thread until \b all given requests are completed
//! \param req_array array of request_ptr objects
//! \param count size of req_array
inline void wait_all(request_ptr req_array[], int count)
{
    wait_all(req_array, req_array + count);
}

//! \brief Cancel requests
//! The specified requests are canceled unless already being processed.
//! However, cancelation cannot be guaranteed.
//! Cancelled requests must still be waited for in order to ensure correct
//! operation.
//! \param reqs_begin begin of request sequence
//! \param reqs_end end of request sequence
//! \return number of request canceled
template <class request_iterator_>
typename std::iterator_traits<request_iterator_>::difference_type cancel_all(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    typename std::iterator_traits<request_iterator_>::difference_type num_canceled = 0;
    while (reqs_begin != reqs_end)
    {
        if ((request_ptr(*reqs_begin))->cancel())
            ++num_canceled;
        ++reqs_begin;
    }
    return num_canceled;
}

//! \brief Polls requests
//! \param reqs_begin begin of request sequence to poll
//! \param reqs_end end of request sequence to poll
//! \param index contains index of the \b first completed request if any
//! \return \c true if any of requests is completed, then index contains valid value, otherwise \c false
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


//! \brief Polls requests
//! \param req_array array of request_ptr objects
//! \param count size of req_array
//! \param index contains index of the \b first completed request if any
//! \return \c true if any of requests is completed, then index contains valid value, otherwise \c false
inline bool poll_any(request_ptr req_array[], int count, int & index)
{
    request_ptr * res = poll_any(req_array, req_array + count);
    index = res - req_array;
    return res != (req_array + count);
}


//! \brief Suspends calling thread until \b any of requests is completed
//! \param reqs_begin begin of request sequence to wait for
//! \param reqs_end end of request sequence to wait for
//! \return index in req_array pointing to the \b first completed request
template <class request_iterator_>
request_iterator_ wait_any(request_iterator_ reqs_begin, request_iterator_ reqs_end)
{
    stats::scoped_wait_timer wait_timer(stats::WAIT_OP_ANY);

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


//! \brief Suspends calling thread until \b any of requests is completed
//! \param req_array array of \c request_ptr objects
//! \param count size of req_array
//! \return index in req_array pointing to the \b first completed request
inline int wait_any(request_ptr req_array[], int count)
{
    return wait_any(req_array, req_array + count) - req_array;
}

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_REQUEST_HEADER
// vim: et:ts=4:sw=4
