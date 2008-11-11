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
#include <stxxl/bits/parallel.h>

#ifndef BOOST_MSVC
// mmap call does not exist in Windows


__STXXL_BEGIN_NAMESPACE


void mmap_request::serve()
{
    // static_cast<syscall_file*>(file_)->set_size(offset+bytes);

    try
    {
        stats::scoped_read_write_timer read_write_timer(bytes, type == WRITE);

 #ifdef STXXL_MMAP_EXPERIMENT1
        int prot = (type == READ) ? PROT_READ : PROT_WRITE;

        void * mem = mmap(buffer, bytes, prot, MAP_SHARED | MAP_FIXED, static_cast<mmap_file *>(file_)->get_file_des(), offset);
        //STXXL_MSG("Mmaped to "<<mem<<" , buffer suggested at "<<((void*)buffer));
        if (mem == MAP_FAILED)
        {
            STXXL_FORMAT_ERROR_MSG(msg, "Mapping failed. " <<
                                   "Page size: " << sysconf(_SC_PAGESIZE) << " offset modulo page size " <<
                                   (offset % sysconf(_SC_PAGESIZE)))

            error_occured(msg.str());
        }
        else if (mem == 0)
        {
            stxxl_function_error(io_error)
        }
        else
        {
            if (type == READ)
            {
                // FIXME: cleanup
                //stxxl_ifcheck (memcpy (buffer, mem, bytes))
                //else
                //stxxl_ifcheck (munmap ((char *) mem, bytes))
            }
            else
            {
                // FIXME: cleanup
                //stxxl_ifcheck (memcpy (mem, buffer, bytes))
                //else
                //stxxl_ifcheck (munmap ((char *) mem, bytes))
            }
        }
 #else
        int prot = (type == READ) ? PROT_READ : PROT_WRITE;
        void * mem = mmap(NULL, bytes, prot, MAP_SHARED, static_cast<mmap_file *>(file_)->get_file_des(), offset);
        // void *mem = mmap (buffer, bytes, prot , MAP_SHARED|MAP_FIXED , static_cast<syscall_file*>(file_)->get_file_des (), offset);
        // STXXL_MSG("Mmaped to "<<mem<<" , buffer suggested at "<<((void*)buffer));
        if (mem == MAP_FAILED)
        {
            STXXL_FORMAT_ERROR_MSG(msg, "Mapping failed. " <<
                                   "Page size: " << sysconf(_SC_PAGESIZE) << " offset modulo page size " <<
                                   (offset % sysconf(_SC_PAGESIZE)));

            error_occured(msg.str());
        }
        else if (mem == 0)
        {
            stxxl_function_error(io_error);
        }
        else
        {
            if (type == READ)
            {
                memcpy(buffer, mem, bytes);
                stxxl_check_ge_0(munmap((char *)mem, bytes), io_error);
            }
            else
            {
                memcpy(mem, buffer, bytes);
                stxxl_check_ge_0(munmap((char *)mem, bytes), io_error);
            }
        }
 #endif
    }
    catch (const io_error & ex)
    {
        error_occured(ex.what());
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
