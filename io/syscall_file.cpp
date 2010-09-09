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


void syscall_file::serve(const request * req) throw (io_error)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    assert(req->get_file() == this);
    offset_type offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_type bytes = req->get_size();
    request::request_type type = req->get_type();

	stats::scoped_read_write_timer read_write_timer(bytes, type == request::WRITE);

    while (bytes > 0)
    {
#ifdef BOOST_MSVC
    	if (::_lseeki64(file_des, offset, SEEK_SET) < 0)
#else
        if (::lseek(file_des, offset, SEEK_SET) < 0)
#endif
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
			int_type rc;

			if (type == request::READ)
			{
				STXXL_DEBUGMON_DO(io_started(buffer));

				if ((rc = ::read(file_des, buffer, bytes)) <= 0)
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
				bytes -= rc;
				offset += rc;

				STXXL_DEBUGMON_DO(io_finished(buffer));
			}
			else
			{
				STXXL_DEBUGMON_DO(io_started(buffer));

				if ((rc = ::write(file_des, buffer, bytes)) <= 0)
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
				bytes -= rc;
				offset += rc;

				STXXL_DEBUGMON_DO(io_finished(buffer));
			}
		}
    }
}

const char * syscall_file::io_type() const
{
    return "syscall";
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
