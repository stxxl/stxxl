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


__STXXL_BEGIN_NAMESPACE


mem_request::mem_request(
    mem_file * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t,
    completion_handler on_cmpl) :
    basic_waiters_request(on_cmpl, f, buf, off, b, t),
    _state(OP)
{
#ifdef STXXL_CHECK_BLOCK_ALIGNING
    // Direct I/O requires file system block size alignment for file offsets,
    // memory buffer addresses, and transfer(buffer) size must be multiple
    // of the file system block size
    check_alignment();
#endif
}

mem_request::~mem_request()
{
    STXXL_VERBOSE3("mem_request " << static_cast<void *>(this) << ": deletion, cnt: " << ref_cnt);

    assert(_state() == DONE || _state() == READY2DIE);
}

void mem_request::wait()
{
    STXXL_VERBOSE3("mem_request : " << static_cast<void *>(this) << " wait ");

    stats::scoped_wait_timer wait_timer;

    _state.wait_for(READY2DIE);

    check_errors();
}

bool mem_request::poll()
{
    const request_status s = _state();

    check_errors();

    return s == DONE || s == READY2DIE;
}

void mem_request::serve()
{
    check_nref();
    STXXL_VERBOSE2("mem_request::serve(): Buffer at " << ((void *)buffer) <<
                   " offset: " << offset <<
                   " bytes: " << bytes <<
                   ((type == READ) ? " READ" : " WRITE") <<
                   " mem: " << static_cast<mem_file *>(file_)->get_ptr());

    if (type == READ)
    {
        stats::scoped_read_timer read_timer(bytes);
        memcpy(buffer, static_cast<mem_file *>(file_)->get_ptr() + offset, bytes);
    }
    else
    {
        stats::scoped_write_timer write_timer(bytes);
        memcpy(static_cast<mem_file *>(file_)->get_ptr() + offset, buffer, bytes);
    }

    check_nref(true);

    _state.set_to(DONE);

    notify_waiters();

    completed();

    _state.set_to(READY2DIE);
}

const char * mem_request::io_type() const
{
    return "memory";
}

////////////////////////////////////////////////////////////////////////////

mem_file::mem_file(int disk) : file(disk), ptr(NULL), sz(0)
{ }

mem_file::~mem_file()
{
    delete[] ptr;
    ptr = NULL;
}

char * mem_file::get_ptr() const
{
    return ptr;
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
    request_ptr req = new mem_request(this,
                                      buffer, pos, bytes,
                                      request::READ, on_cmpl);

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
    request_ptr req = new mem_request(this, buffer, pos, bytes,
                                      request::WRITE, on_cmpl);

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
        memcpy(get_ptr() + offset, uninitialized, BLOCK_ALIGN);
        offset += BLOCK_ALIGN;
        size -= BLOCK_ALIGN;
    }
    if (size > 0)
        memcpy(get_ptr() + offset, uninitialized, size);
    free(uninitialized);
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
