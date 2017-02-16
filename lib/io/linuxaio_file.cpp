/***************************************************************************
 *  lib/io/linuxaio_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2011 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/linuxaio_file.h>

#if STXXL_HAVE_LINUXAIO_FILE

#include <stxxl/bits/io/linuxaio_request.h>
#include <stxxl/bits/io/disk_queues.h>

namespace stxxl {

request_ptr linuxaio_file::aread(
    void* buffer, offset_type offset, size_type bytes,
    const completion_handler& on_complete)
{
    request_ptr req = make_counting<linuxaio_request>(
        on_complete, this, buffer, offset, bytes, request::READ);

    disk_queues::get_instance()->add_request(req, get_queue_id());

    return req;
}

request_ptr linuxaio_file::awrite(
    void* buffer, offset_type offset, size_type bytes,
    const completion_handler& on_complete)
{
    request_ptr req = make_counting<linuxaio_request>(
        on_complete, this, buffer, offset, bytes, request::WRITE);

    disk_queues::get_instance()->add_request(req, get_queue_id());

    return req;
}

void linuxaio_file::serve(void* buffer, offset_type offset, size_type bytes,
                          request::read_or_write op)
{
    // req need not be an linuxaio_request
    if (op == request::READ)
        aread(buffer, offset, bytes)->wait();
    else
        awrite(buffer, offset, bytes)->wait();
}

const char* linuxaio_file::io_type() const
{
    return "linuxaio";
}

} // namespace stxxl

#endif // #if STXXL_HAVE_LINUXAIO_FILE
// vim: et:ts=4:sw=4
