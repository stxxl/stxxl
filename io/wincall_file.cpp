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

#if STXXL_HAVE_WINCALL_FILE

#include <stxxl/bits/io/request_impl_basic.h>
#include <stxxl/bits/common/debug.h>

#include <windows.h>

__STXXL_BEGIN_NAMESPACE


void wincall_file::serve(const request * req) throw (io_error)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    assert(req->get_file() == this);
    offset_type offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_type bytes = req->get_size();
    request::request_type type = req->get_type();

    if (bytes > 32 * 1024 * 1024) {
        STXXL_ERRMSG("Using a block size larger than 32 MiB may not work with the " << io_type() << " filetype");
    }

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
            } else if (NumberOfBytesRead != bytes) {
                stxxl_win_lasterror_exit(" partial read: missing " << (bytes - NumberOfBytesRead) << " out of " << bytes << " bytes",
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
            } else if (NumberOfBytesWritten != bytes) {
                stxxl_win_lasterror_exit(" partial write: missing " << (bytes - NumberOfBytesWritten) << " out of " << bytes << " bytes",
                                         io_error);
            }

            STXXL_DEBUGMON_DO(io_finished(buffer));
        }
    }
}

const char * wincall_file::io_type() const
{
    return "wincall";
}

__STXXL_END_NAMESPACE

#endif  // #if STXXL_HAVE_WINCALL_FILE
// vim: et:ts=4:sw=4
