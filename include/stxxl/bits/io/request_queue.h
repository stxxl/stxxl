/***************************************************************************
 *  include/stxxl/bits/io/request_queue.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2011 Johannes Singler <singler@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_REQUEST_QUEUE_HEADER
#define STXXL_IO_REQUEST_QUEUE_HEADER

#include <stxxl/bits/io/request.h>

namespace stxxl {

//! \addtogroup reqlayer
//! \{

//! Interface of a request_queue to which requests can be added and canceled.
class request_queue
{
public:
    enum priority_op { READ, WRITE, NONE };

public:
    request_queue() = default;

    //! non-copyable: delete copy-constructor
    request_queue(const request_queue&) = delete;
    //! non-copyable: delete assignment operator
    request_queue& operator = (const request_queue&) = delete;

    virtual void add_request(request_ptr& req) = 0;
    virtual bool cancel_request(request_ptr& req) = 0;
    virtual ~request_queue() { }
    virtual void set_priority_op(const priority_op& p) { STXXL_UNUSED(p); }
};

//! \}

} // namespace stxxl

#endif // !STXXL_IO_REQUEST_QUEUE_HEADER
// vim: et:ts=4:sw=4
