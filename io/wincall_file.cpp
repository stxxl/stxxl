/***************************************************************************
 *  io/wincall_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2005-2006 Roman Dementiev <dementiev@ira.uka.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/wincall_file.h>
#include <stxxl/bits/common/debug.h>

#ifdef BOOST_MSVC

__STXXL_BEGIN_NAMESPACE


wincall_request::wincall_request(
    wincall_file * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t,
    completion_handler on_cmpl) :
    wfs_request_base(f, buf, off, b, t, on_cmpl)
{ }

void wincall_file::serve(const request * req) throw(io_error)
{
    assert(req->get_file() == this);
    stxxl::int64 offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_t bytes = req->get_size();
    request::request_type type = req->get_type();

        HANDLE handle = file_des;
        LARGE_INTEGER desired_pos;
        desired_pos.QuadPart = offset;
        if (!SetFilePointerEx(handle, desired_pos, NULL, FILE_BEGIN))
        {
            stxxl_win_lasterror_exit("SetFilePointerEx in wincall_request::serve()" <<
                                     " offset=" << offset <<
                                     " this=" << this <<
                                     " buffer=" << buffer <<
                                     " bytes=" << bytes <<
                                     " type=" << ((type == request::READ) ? "READ" : "WRITE"),
                                     io_error);
        }
        else
        {
            stats::scoped_read_write_timer read_write_timer(bytes, type == request::WRITE);

            if (type == request::READ)
            {
                STXXL_DEBUGMON_DO(io_started(buffer));
                DWORD NumberOfBytesRead = 0;
                if (!ReadFile(handle, buffer, bytes, &NumberOfBytesRead, NULL))
                {
                    stxxl_win_lasterror_exit("ReadFile" <<
                                             " this=" << this <<
                                             " offset=" << offset <<
                                             " buffer=" << buffer <<
                                             " bytes=" << bytes <<
                                             " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                                             " NumberOfBytesRead= " << NumberOfBytesRead,
                                             io_error);
                }

                STXXL_DEBUGMON_DO(io_finished(buffer));
            }
            else
            {
                STXXL_DEBUGMON_DO(io_started(buffer));

                DWORD NumberOfBytesWritten = 0;
                if (!WriteFile(handle, buffer, bytes, &NumberOfBytesWritten, NULL))
                {
                    stxxl_win_lasterror_exit("WriteFile" <<
                                             " this=" << this <<
                                             " offset=" << offset <<
                                             " buffer=" << buffer <<
                                             " bytes=" << bytes <<
                                             " type=" << ((type == request::READ) ? "READ" : "WRITE") <<
                                             " NumberOfBytesWritten= " << NumberOfBytesWritten,
                                             io_error);
                }

                STXXL_DEBUGMON_DO(io_finished(buffer));
            }
        }
}

void wincall_request::serve()
{
    check_nref();
    STXXL_VERBOSE2("wincall_request::serve():" <<
                   " Buffer at " << buffer <<
                   " offset: " << offset <<
                   " bytes: " << bytes <<
                   ((type == request::READ) ? " READ" : " WRITE"));

    try {
        file_->serve(this);
    }
    catch (const io_error & ex)
    {
        error_occured(ex.what());
    }

    check_nref(true);

    _state.set_to(DONE);
    completed();
    _state.set_to(READY2DIE);
}

const char * wincall_file::io_type() const
{
    return "wincall";
}

////////////////////////////////////////////////////////////////////////////

wincall_file::wincall_file(
    const std::string & filename,
    int mode,
    int disk) : wfs_file_base(filename, mode, disk)
{ }

request_ptr wincall_file::aread(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new wincall_request(this,
                                          buffer, pos, bytes,
                                          request::READ, on_cmpl);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_readreq(req, get_id());

    return req;
}

request_ptr wincall_file::awrite(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new wincall_request(this, buffer, pos, bytes,
                                          request::WRITE, on_cmpl);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_writereq(req, get_id());

    return req;
}

__STXXL_END_NAMESPACE

#endif // #ifdef BOOST_MSVC
// vim: et:ts=4:sw=4
