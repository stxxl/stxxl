/***************************************************************************
 *  include/stxxl/bits/io/completion_handler.h
 *
 *  Loki-style completion handler (functors)
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMPLETION_HANDLER_HEADER
#define STXXL_COMPLETION_HANDLER_HEADER

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/compat_unique_ptr.h>


__STXXL_BEGIN_NAMESPACE

class request;

class completion_handler_impl
{
public:
    virtual void operator () (request *) = 0;
    virtual completion_handler_impl * clone() const = 0;
    virtual ~completion_handler_impl() { }
};

template <typename handler_type>
class completion_handler1 : public completion_handler_impl
{
private:
    handler_type handler_;

public:
    completion_handler1(const handler_type & handler__) : handler_(handler__) { }
    completion_handler1 * clone() const
    {
        return new completion_handler1(*this);
    }
    void operator () (request * req)
    {
        handler_(req);
    }
};

//! \brief Completion handler class (Loki-style)

//! In some situations one needs to execute
//! some actions after completion of an I/O
//! request. In these cases one can use
//! an I/O completion handler - a function
//! object that can be passed as a parameter
//! to asynchronous I/O calls \c stxxl::file::aread
//! and \c stxxl::file::awrite .
//! For an example of use see \link mng/test_mng.cpp mng/test_mng.cpp \endlink
class completion_handler
{
    compat_unique_ptr<completion_handler_impl>::result sp_impl_;

public:
    completion_handler() :
        sp_impl_(static_cast<completion_handler_impl *>(0))
    { }

    completion_handler(const completion_handler & obj) :
        sp_impl_(obj.sp_impl_.get()->clone())
    { }

    template <typename handler_type>
    completion_handler(const handler_type & handler__) :
        sp_impl_(new completion_handler1<handler_type>(handler__))
    { }

    completion_handler & operator = (const completion_handler & obj)
    {
        sp_impl_.reset(obj.sp_impl_.get()->clone());
        return *this;
    }
    void operator () (request * req)
    {
        (*sp_impl_)(req);
    }
};

//! \brief Default completion handler class

struct default_completion_handler
{
    //! \brief An operator that does nothing
    void operator () (request *) { }
};

__STXXL_END_NAMESPACE

#endif // !STXXL_COMPLETION_HANDLER_HEADER
// vim: et:ts=4:sw=4
