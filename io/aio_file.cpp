/***************************************************************************
 *  io/aio_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2009 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#include <stxxl/bits/io/aio_file.h>

#if STXXL_HAVE_AIO_FILE

#include <stxxl/bits/io/aio_request.h>

#include <aio.h>

__STXXL_BEGIN_NAMESPACE

void aio_file::serve(const request* req) throw(io_error)
{
	STXXL_UNUSED(req);
	assert(false);
}

const char* aio_file::io_type() const
{
    return "aio";
}

request_ptr aio_file::aread(
    void* buffer,
    offset_type pos,
    size_type bytes,
    const completion_handler& on_cmpl)
{
    request_ptr req = new aio_request(on_cmpl, this, buffer, pos, bytes, request::READ);

    aio_queue::get_instance()->add_request(req);

    return req;
}

request_ptr aio_file::awrite(
    void* buffer,
    offset_type pos,
    size_type bytes,
    const completion_handler& on_cmpl)
{
    request_ptr req = new aio_request(on_cmpl, this, buffer, pos, bytes, request::WRITE);

    aio_queue::get_instance()->add_request(req);

    return req;
}

__STXXL_END_NAMESPACE

#endif // #ifndef BOOST_MSVC
// vim: et:ts=4:sw=4
