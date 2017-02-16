/***************************************************************************
 *  lib/io/syscall_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/common/error_handling.h>
#include <stxxl/bits/config.h>
#include <stxxl/bits/io/iostats.h>
#include <stxxl/bits/io/request.h>
#include <stxxl/bits/io/request_interface.h>
#include <stxxl/bits/io/syscall_file.h>
#include "ufs_platform.h"

#include <mutex>
#include <limits>

namespace stxxl {

void syscall_file::serve(void* buffer, offset_type offset, size_type bytes,
                         request::read_or_write op)
{
    std::unique_lock<std::mutex> fd_lock(fd_mutex_);

    char* cbuffer = static_cast<char*>(buffer);

    stats::scoped_read_write_timer read_write_timer(bytes, op == request::WRITE);

    while (bytes > 0)
    {
        off_t rc = ::lseek(file_des_, offset, SEEK_SET);
        if (rc < 0)
        {
            STXXL_THROW_ERRNO
                (io_error,
                " this=" << this <<
                " call=::lseek(fd,offset,SEEK_SET)" <<
                " path=" << filename_ <<
                " fd=" << file_des_ <<
                " offset=" << offset <<
                " buffer=" << cbuffer <<
                " bytes=" << bytes <<
                " op=" << ((op == request::READ) ? "READ" : "WRITE") <<
                " rc=" << rc);
        }

        if (op == request::READ)
        {
#if STXXL_MSVC
            assert(bytes <= std::numeric_limits<unsigned int>::max());
            if ((rc = ::read(file_des_, cbuffer, (unsigned int)bytes)) <= 0)
#else
            if ((rc = ::read(file_des_, cbuffer, bytes)) <= 0)
#endif
            {
                STXXL_THROW_ERRNO
                    (io_error,
                    " this=" << this <<
                    " call=::read(fd,buffer,bytes)" <<
                    " path=" << filename_ <<
                    " fd=" << file_des_ <<
                    " offset=" << offset <<
                    " buffer=" << buffer <<
                    " bytes=" << bytes <<
                    " op=" << "READ" <<
                    " rc=" << rc);
            }
            bytes = (size_type)(bytes - rc);
            offset += rc;
            cbuffer += rc;

            if (bytes > 0 && offset == this->_size())
            {
                // read request extends past end-of-file
                // fill reminder with zeroes
                memset(cbuffer, 0, bytes);
                bytes = 0;
            }
        }
        else
        {
#if STXXL_MSVC
            assert(bytes <= std::numeric_limits<unsigned int>::max());
            if ((rc = ::write(file_des_, cbuffer, (unsigned int)bytes)) <= 0)
#else
            if ((rc = ::write(file_des_, cbuffer, bytes)) <= 0)
#endif
            {
                STXXL_THROW_ERRNO
                    (io_error,
                    " this=" << this <<
                    " call=::write(fd,buffer,bytes)" <<
                    " path=" << filename_ <<
                    " fd=" << file_des_ <<
                    " offset=" << offset <<
                    " buffer=" << buffer <<
                    " bytes=" << bytes <<
                    " op=" << "WRITE" <<
                    " rc=" << rc);
            }
            bytes = (size_type)(bytes - rc);
            offset += rc;
            cbuffer += rc;
        }
    }
}

const char* syscall_file::io_type() const
{
    return "syscall";
}

} // namespace stxxl
// vim: et:ts=4:sw=4
