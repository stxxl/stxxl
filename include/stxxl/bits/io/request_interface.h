/***************************************************************************
 *  include/stxxl/bits/io/request_interface.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2009, 2011 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO__REQUEST_INTERFACE_H_
#define STXXL_IO__REQUEST_INTERFACE_H_

#include <ostream>

#include <stxxl/bits/namespace.h>
#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/common/types.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

class onoff_switch;

//! \brief Functional interface of a request

//! Since all library I/O operations are asynchronous,
//! one needs to keep track of their status:
//! e.g. whether an I/O operation completed or not.
class request_interface : private noncopyable
{
public:
    typedef stxxl::external_size_type offset_type;
    typedef stxxl::internal_size_type size_type;
    enum request_type { READ, WRITE };

public:
    virtual bool add_waiter(onoff_switch * sw) = 0;
    virtual void delete_waiter(onoff_switch * sw) = 0;

protected:
    virtual void notify_waiters() = 0;

public:
    // HACK!
    virtual void serve() = 0;

protected:
    virtual void completed() = 0;

public:
    //! \brief Suspends calling thread until completion of the request
    virtual void wait(bool measure_time = true) = 0;

    //! \brief Cancel a request.
    //!
    //! The request is canceled unless already being processed.
    //! However, cancelation cannot be guaranteed.
    //! Canceled requests must still be waited for in order to ensure correct operation.
    //! If the request was canceled successfully, the completion handler will not be called.
    //! \return \c true iff the request was canceled successfully
    virtual bool cancel() = 0;

    //! \brief Polls the status of the request
    //! \return \c true if request is completed, otherwise \c false
    virtual bool poll() = 0;

    //! \brief Identifies the type of I/O implementation
    //! \return pointer to null terminated string of characters, containing the name of I/O implementation
    virtual const char * io_type() const = 0;

    //! \brief Dumps properties of a request
    virtual std::ostream & print(std::ostream & out) const = 0;

    virtual ~request_interface()
    { }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO__REQUEST_INTERFACE_H_
// vim: et:ts=4:sw=4
