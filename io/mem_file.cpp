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
#include <stxxl/bits/common/debug.h>
#include <stxxl/bits/parallel.h>


__STXXL_BEGIN_NAMESPACE


mem_request::mem_request(
    mem_file * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t,
    completion_handler on_cmpl) :
    request(on_cmpl, f, buf, off, b, t)
{
#ifdef STXXL_CHECK_BLOCK_ALIGNING
    // Direct I/O requires file system block size alignment for file offsets,
    // memory buffer addresses, and transfer(buffer) size must be multiple
    // of the file system block size
    check_aligning();
#endif
}

mem_request::~mem_request()
{
    STXXL_VERBOSE3("mem_request " << static_cast<void *>(this) << ": deletion, cnt: " << ref_cnt);

    assert(_state() == DONE || _state() == READY2DIE);
}

bool mem_request::add_waiter(onoff_switch * sw)
{
    scoped_mutex_lock Lock(waiters_mutex);

    if (poll())                     // request already finished
    {
        return true;
    }

    waiters.insert(sw);

    return false;
}

void mem_request::delete_waiter(onoff_switch * sw)
{
    scoped_mutex_lock Lock(waiters_mutex);
    waiters.erase(sw);
}

int mem_request::nwaiters()                 // returns number of waiters
{
    scoped_mutex_lock Lock(waiters_mutex);
    return waiters.size();
}

void mem_request::check_aligning()
{
    if (offset % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Offset is not aligned: modulo "
                                              << BLOCK_ALIGN << " = " <<
                     offset % BLOCK_ALIGN);

    if (bytes % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Size is not a multiple of " <<
                     BLOCK_ALIGN << ", = " << bytes % BLOCK_ALIGN);

    if (long(buffer) % BLOCK_ALIGN != 0)
        STXXL_ERRMSG("Buffer is not aligned: modulo "
                                              << BLOCK_ALIGN << " = " <<
                     long(buffer) % BLOCK_ALIGN << " (" <<
                     std::hex << buffer << std::dec << ")");
}

void mem_request::wait()
{
    STXXL_VERBOSE3("mem_request : " << static_cast<void *>(this) << " wait ");

    START_COUNT_WAIT_TIME

#ifdef NO_OVERLAPPING
    enqueue();
#endif

    _state.wait_for(READY2DIE);

    END_COUNT_WAIT_TIME

    check_errors();
}

bool mem_request::poll()
{
#ifdef NO_OVERLAPPING
    /*if(_state () < DONE)*/ wait();
#endif

    bool s = _state() >= DONE;

    check_errors();

    return s;
}

void mem_request::serve()
{
    if (nref() < 2)
    {
        STXXL_ERRMSG("WARNING: serious error, reference to the request is lost before serve" <<
                     " (nref=" << nref() << ") " <<
                     " this=" << long(this) <<
                     " mem address=" << static_cast<mem_file *>(file_)->get_ptr() <<
                     " mem size=" << static_cast<mem_file *>(file_)->size() <<
                     " offset=" << offset <<
                     " buffer=" << buffer <<
                     " bytes=" << bytes <<
                     " type=" << ((type == READ) ? "READ" : "WRITE"));
    }
    STXXL_VERBOSE2("mem_request::serve(): Buffer at " << ((void *)buffer) <<
                   " offset: " << offset <<
                   " bytes: " << bytes <<
                   ((type == READ) ? " READ" : " WRITE") <<
                   " mem: " << static_cast<mem_file *>(file_)->get_ptr());

    if (type == READ)
    {
        stats::scoped_read_timer read_timer(size());
        memcpy(buffer, static_cast<mem_file *>(file_)->get_ptr() + offset, bytes);
    }
    else
    {
        stats::scoped_write_timer write_timer(size());
        memcpy(static_cast<mem_file *>(file_)->get_ptr() + offset, buffer, bytes);
    }

    if (nref() < 2)
    {
        STXXL_ERRMSG("WARNING: reference to the request is lost after serve" <<
                     " (nref=" << nref() << ") " <<
                     " this=" << long(this) <<
                     " mem address=" << static_cast<mem_file *>(file_)->get_ptr() <<
                     " mem size=" << static_cast<mem_file *>(file_)->size() <<
                     " offset=" << offset <<
                     " buffer=" << buffer <<
                     " bytes=" << bytes <<
                     " type=" << ((type == READ) ? "READ" : "WRITE"));
    }

    _state.set_to(DONE);

    {
        scoped_mutex_lock Lock(waiters_mutex);

        // << notification >>
        std::for_each(
            waiters.begin(),
            waiters.end(),
            std::mem_fun(&onoff_switch::on)
            __STXXL_FORCE_SEQUENTIAL);
    }

    completed();

    _state.set_to(READY2DIE);
}

const char * mem_request::io_type()
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

#ifndef NO_OVERLAPPING
    disk_queues::get_instance()->add_readreq(req, get_id());
#endif
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

#ifndef NO_OVERLAPPING
    disk_queues::get_instance()->add_writereq(req, get_id());
#endif
    return req;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
