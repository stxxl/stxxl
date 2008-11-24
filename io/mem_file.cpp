/***************************************************************************
 *  io/mem_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/mem_file.h>
#include <stxxl/bits/io/request_impl_basic.h>


__STXXL_BEGIN_NAMESPACE


void mem_file::serve(const request * req) throw(io_error)
{
    assert(req->get_file() == this);
    stxxl::int64 offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_t bytes = req->get_size();
    request::request_type type = req->get_type();

    if (type == request::READ)
    {
        stats::scoped_read_timer read_timer(bytes);
        memcpy(buffer, ptr + offset, bytes);
    }
    else
    {
        stats::scoped_write_timer write_timer(bytes);
        memcpy(ptr + offset, buffer, bytes);
    }
}

const char * mem_file::io_type() const
{
    return "memory";
}

mem_file::~mem_file()
{
    delete[] ptr;
    ptr = NULL;
}

void mem_file::lock()
{
    // nothing to do
}

stxxl::int64 mem_file::size()
{
    return sz;
}

void mem_file::set_size(stxxl::int64 newsize)
{
    assert(ptr == NULL); // no resizing
    if (ptr == NULL)
        ptr = new char[sz = newsize];
}

request_ptr mem_file::aread(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new request_impl_basic(on_cmpl, this,
                                      buffer, pos, bytes,
                                      request::READ);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_readreq(req, get_id());

    return req;
}

request_ptr mem_file::awrite(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new request_impl_basic(on_cmpl, this, buffer, pos, bytes,
                                      request::WRITE);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_writereq(req, get_id());

    return req;
}

void mem_file::delete_region(int64 offset, unsigned_type size)
{
    // overwrite the freed region with uninitialized memory
    STXXL_VERBOSE("delete_region at " << offset << " len " << size);
    void * uninitialized = malloc(BLOCK_ALIGN);
    while (size >= BLOCK_ALIGN) {
        memcpy(ptr + offset, uninitialized, BLOCK_ALIGN);
        offset += BLOCK_ALIGN;
        size -= BLOCK_ALIGN;
    }
    if (size > 0)
        memcpy(ptr + offset, uninitialized, size);
    free(uninitialized);
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
