/***************************************************************************
 *  io/mmap_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/mmap_file.h>

#ifndef BOOST_MSVC
// mmap call does not exist in Windows


__STXXL_BEGIN_NAMESPACE


void mmap_file::serve(const request * req) throw(io_error)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    assert(req->get_file() == this);
    stxxl::int64 offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_t bytes = req->get_size();
    request::request_type type = req->get_type();

        stats::scoped_read_write_timer read_write_timer(bytes, type == request::WRITE);

        int prot = (type == request::READ) ? PROT_READ : PROT_WRITE;
        void * mem = mmap(NULL, bytes, prot, MAP_SHARED, file_des, offset);
        // void *mem = mmap (buffer, bytes, prot , MAP_SHARED|MAP_FIXED , file_des, offset);
        // STXXL_MSG("Mmaped to "<<mem<<" , buffer suggested at "<<buffer);
        if (mem == MAP_FAILED)
        {
            STXXL_THROW2(io_error,
                         " Mapping failed." <<
                         " Page size: " << sysconf(_SC_PAGESIZE) <<
                         " offset modulo page size " << (offset % sysconf(_SC_PAGESIZE)));
        }
        else if (mem == 0)
        {
            stxxl_function_error(io_error);
        }
        else
        {
            if (type == request::READ)
            {
                memcpy(buffer, mem, bytes);
            }
            else
            {
                memcpy(mem, buffer, bytes);
            }
            stxxl_check_ge_0(munmap(mem, bytes), io_error);
        }
}

void mmap_request::serve()
{
    try
    {
        file_->serve(this);
    }
    catch (const io_error & ex)
    {
        error_occured(ex.what());
    }

    _state.set_to(DONE);
    completed();
    _state.set_to(READY2DIE);
}

const char * mmap_file::io_type() const
{
    return "mmap";
}

////////////////////////////////////////////////////////////////////////////

request_ptr mmap_file::aread(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new mmap_request(
        this,
        buffer,
        pos,
        bytes,
        request::READ,
        on_cmpl);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_readreq(req, get_id());

    return req;
}

request_ptr mmap_file::awrite(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new mmap_request(
        this,
        buffer,
        pos,
        bytes,
        request::WRITE,
        on_cmpl);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_writereq(req, get_id());

    return req;
}

__STXXL_END_NAMESPACE

#endif // #ifndef BOOST_MSVC
// vim: et:ts=4:sw=4
