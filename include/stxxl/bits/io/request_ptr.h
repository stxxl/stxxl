/***************************************************************************
 *  include/stxxl/bits/io/request_ptr.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO__REQUEST_PTR_H_
#define STXXL_IO__REQUEST_PTR_H_

#include <cassert>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/verbose.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

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

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO__REQUEST_PTR_H_
// vim: et:ts=4:sw=4
