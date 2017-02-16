/***************************************************************************
 *  lib/io/disk_queued_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/disk_queued_file.h>
#include <stxxl/bits/io/disk_queues.h>
#include <stxxl/bits/io/file.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/io/serving_request.h>
#include <stxxl/bits/singleton.h>

namespace stxxl {

request_ptr disk_queued_file::aread(
    void* buffer, offset_type offset, size_type bytes,
    const completion_handler& on_complete)
{
    request_ptr req = make_counting<serving_request>(
        on_complete, this, buffer, offset, bytes, request::READ);

    disk_queues::get_instance()->add_request(req, get_queue_id());

    return req;
}

request_ptr disk_queued_file::awrite(
    void* buffer, offset_type offset, size_type bytes,
    const completion_handler& on_complete)
{
    request_ptr req = make_counting<serving_request>(
        on_complete, this, buffer, offset, bytes, request::WRITE);

    disk_queues::get_instance()->add_request(req, get_queue_id());

    return req;
}

} // namespace stxxl
// vim: et:ts=4:sw=4
