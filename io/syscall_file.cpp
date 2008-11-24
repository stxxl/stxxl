/***************************************************************************
 *  io/syscall_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/syscall_file.h>
#include <stxxl/bits/io/request_impl_basic.h>
#include <stxxl/bits/common/debug.h>


__STXXL_BEGIN_NAMESPACE


void syscall_file::serve(const request * req) throw(io_error)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    assert(req->get_file() == this);
    stxxl::int64 offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_t bytes = req->get_size();
    request::request_type type = req->get_type();

        if (::lseek(file_des, offset, SEEK_SET) < 0)
        {
            STXXL_THROW2(io_error,
                         " this=" << this <<
                         " call=::lseek(fd,offset,SEEK_SET)" <<
                         " fd=" << file_des <<
                         " offset=" << offset <<
                         " buffer=" << buffer <<
                         " bytes=" << bytes <<
                         " type=" << ((type == request::READ) ? "READ" : "WRITE"));
        }
        else
        {
            stats::scoped_read_write_timer read_write_timer(bytes, type == request::WRITE);

            if (type == request::READ)
            {
                STXXL_DEBUGMON_DO(io_started(buffer));

                if (::read(file_des, buffer, bytes) < 0)
                {
                    STXXL_THROW2(io_error,
                                 " this=" << this <<
                                 " call=::read(fd,buffer,bytes)" <<
                                 " fd=" << file_des <<
                                 " offset=" << offset <<
                                 " buffer=" << buffer <<
                                 " bytes=" << bytes <<
                                 " type=" << ((type == request::READ) ? "READ" : "WRITE"));
                }

                STXXL_DEBUGMON_DO(io_finished(buffer));
            }
            else
            {
                STXXL_DEBUGMON_DO(io_started(buffer));

                if (::write(file_des, buffer, bytes) < 0)
                {
                    STXXL_THROW2(io_error,
                                 " this=" << this <<
                                 " call=::write(fd,buffer,bytes)" <<
                                 " fd=" << file_des <<
                                 " offset=" << offset <<
                                 " buffer=" << buffer <<
                                 " bytes=" << bytes <<
                                 " type=" << ((type == request::READ) ? "READ" : "WRITE"));
                }

                STXXL_DEBUGMON_DO(io_finished(buffer));
            }
        }
}

const char * syscall_file::io_type() const
{
    return "syscall";
}

request_ptr syscall_file::aread(
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

request_ptr syscall_file::awrite(
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

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
