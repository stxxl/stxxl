/***************************************************************************
 *  include/stxxl/bits/io/request_queue.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_IO_REQUEST_QUEUE_HEADER
#define STXXL_IO_REQUEST_QUEUE_HEADER

#include <stxxl/bits/noncopyable.h>
#include <stxxl/bits/io/request.h>


__STXXL_BEGIN_NAMESPACE

//! \addtogroup iolayer
//! \{

class disk_queue
{
public:
    enum priority_op { READ, WRITE, NONE };
};

class request_queue : private noncopyable
{
public:
    virtual void add_request(request_ptr & req) = 0;
    virtual bool cancel_request(request_ptr & req) = 0;
    virtual ~request_queue() { }
};

//! \}

__STXXL_END_NAMESPACE

#endif // !STXXL_IO_REQUEST_QUEUE_HEADER
// vim: et:ts=4:sw=4
