/***************************************************************************
 *  io/mmap_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/mmap_file.h>

#if STXXL_HAVE_MMAP_FILE

#include <stxxl/bits/io/request_impl_basic.h>
#include <stxxl/bits/io/iostats.h>


__STXXL_BEGIN_NAMESPACE


void mmap_file::serve(const request * req) throw (io_error)
{
    scoped_mutex_lock fd_lock(fd_mutex);
    assert(req->get_file() == this);
    offset_type offset = req->get_offset();
    void * buffer = req->get_buffer();
    size_type bytes = req->get_size();
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

const char * mmap_file::io_type() const
{
    return "mmap";
}

__STXXL_END_NAMESPACE

#endif  // #if STXXL_HAVE_MMAP_FILE
// vim: et:ts=4:sw=4
