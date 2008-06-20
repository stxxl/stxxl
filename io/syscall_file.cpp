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


__STXXL_BEGIN_NAMESPACE

syscall_file::syscall_file(
    const std::string & filename,
    int mode,
    int disk) : ufs_file_base(filename, mode, disk)
{ }

syscall_request::syscall_request(
    syscall_file * f,
    void * buf,
    stxxl::int64 off,
    size_t b,
    request_type t,
    completion_handler on_cmpl) :
    ufs_request_base(f, buf, off, b, t, on_cmpl)
{ }
const char * syscall_request::io_type()
{
    return "syscall";
}


void syscall_request::serve()
{
    stats * iostats = stats::get_instance();
    if (nref() < 2)
    {
        STXXL_ERRMSG("WARNING: serious error, reference to the request is lost before serve (nref="
                                << nref() << ") " <<
                     " this=" << long(this) <<
                     " File descriptor=" <<
                     static_cast<syscall_file *>(file_)->get_file_des() << " offset=" << offset << " buffer=" << buffer << " bytes=" << bytes
                                << " type=" << ((type == READ) ? "READ" : "WRITE"));
    }
    STXXL_VERBOSE2("syscall_request::serve(): Buffer at " << ((void *)buffer)
                                                          << " offset: " << offset << " bytes: " << bytes << ((type == READ) ? " READ" : " WRITE")
                                                          << " file: " << static_cast<syscall_file *>(file_)->get_file_des());

    try
    {
        if (::lseek(static_cast<syscall_file *>(file_)->get_file_des(), offset, SEEK_SET) < 0)
        {
            STXXL_THROW2(io_error,
                         " this=" << long(this) << " File descriptor=" <<
                         static_cast<syscall_file *>(file_)->get_file_des() << " offset=" << offset << " buffer=" <<
                         buffer << " bytes=" << bytes
                                  << " type=" << ((type == READ) ? "READ" : "WRITE"));
        }
        else
        {
            if (type == READ)
            {
#if STXXL_IO_STATS
                iostats->read_started(size());
#endif

                debugmon::get_instance()->io_started((char *)buffer);

                if (::read(static_cast<syscall_file *>(file_)->get_file_des(), buffer, bytes) < 0)
                {
                    STXXL_THROW2(io_error,
                                 " this=" << long(this) << " File descriptor=" <<
                                 static_cast<syscall_file *>(file_)->get_file_des() << " offset=" << offset <<
                                 " buffer=" << buffer << " bytes=" << bytes << " type=" <<
                                 ((type == READ) ? "READ" : "WRITE") << " nref= " << nref());
                }

                debugmon::get_instance()->io_finished((char *)buffer);

#if STXXL_IO_STATS
                iostats->read_finished();
#endif
            }
            else
            {
#if STXXL_IO_STATS
                iostats->write_started(size());
#endif

                debugmon::get_instance()->io_started((char *)buffer);

                if (::write(static_cast<syscall_file *>(file_)->get_file_des(), buffer, bytes) < 0)
                {
                    STXXL_THROW2(io_error,
                                 " this=" << long(this) << " File descriptor=" <<
                                 static_cast<syscall_file *>(file_)->get_file_des() << " offset=" << offset << " buffer=" <<
                                 buffer << " bytes=" << bytes << " type=" <<
                                 ((type == READ) ? "READ" : "WRITE") << " nref= " << nref());
                }

                debugmon::get_instance()->io_finished((char *)buffer);

#if STXXL_IO_STATS
                iostats->write_finished();
#endif
            }
        }
    }
    catch (const io_error & ex)
    {
        error_occured(ex.what());
    }


    if (nref() < 2)
    {
        STXXL_ERRMSG("WARNING: reference to the request is lost after serve (nref=" << nref() << ") " <<
                     " this=" << long(this) << " File descriptor=" << static_cast<syscall_file *>(file_)->get_file_des() <<
                     " offset=" << offset << " buffer=" << buffer << " bytes=" << bytes <<
                     " type=" << ((type == READ) ? "READ" : "WRITE"));
    }


    _state.set_to(DONE);


#ifdef STXXL_BOOST_THREADS
    boost::mutex::scoped_lock Lock(waiters_mutex);
#else
    waiters_mutex.lock();
#endif
    // << notification >>
    std::for_each(
        waiters.begin(),
        waiters.end(),
        std::mem_fun(&onoff_switch::on)
        __STXXL_FORCE_SEQUENTIAL);

#ifdef STXXL_BOOST_THREADS
    Lock.unlock();
#else
    waiters_mutex.unlock();
#endif

    completed();

    _state.set_to(READY2DIE);
}

request_ptr syscall_file::aread(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new syscall_request(this,
                                          buffer, pos, bytes,
                                          request::READ, on_cmpl);

    if (!req.get())
        stxxl_function_error(io_error);


#ifndef NO_OVERLAPPING
    disk_queues::get_instance()->add_readreq(req, get_id());
#endif
    return req;
}
request_ptr syscall_file::awrite(
    void * buffer,
    stxxl::int64 pos,
    size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new syscall_request(this, buffer, pos, bytes,
                                          request::WRITE, on_cmpl);

    if (!req.get())
        stxxl_function_error(io_error);

#ifndef NO_OVERLAPPING
    disk_queues::get_instance()->add_writereq(req, get_id());
#endif
    return req;
}

__STXXL_END_NAMESPACE
